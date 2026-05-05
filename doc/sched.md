# 调度器与进程管理

[TOC]

## 1. 概述

调度器子系统负责进程/线程的创建、调度、切换和销毁。采用可插拔 `sched_class` 设计，支持多种调度策略。目前实现了时间片轮转（Round-Robin）和空闲（Idle）两种调度类。

核心特性：
- **可插拔调度类**：`sched_class` 接口支持扩展新调度策略
- **per-CPU 运行队列**：每个 CPU 独立的 `struct rq`
- **时间片抢占**：基于 timer 的时间片到期自动触发重调度
- **进程管理**：fork / execve / exit / waitpid 完整生命周期
- **内核线程**：`kernel_thread()` 创建内核态线程

## 2. 调度类（sched_class）

```c
struct sched_class {
    const char *name;
    const struct sched_class *next;
    void (*enqueue_task)(struct rq *rq, struct task_struct *p);
    void (*dequeue_task)(struct rq *rq, struct task_struct *p);
    struct task_struct *(*pick_next_task)(struct rq *rq);
    void (*task_yield)(struct rq *rq, struct task_struct *curr);
};
```

调度优先级链：`rr_sched_class → idle_sched_class → NULL`

`sched()` 函数从高到低遍历调度类，找到第一个有可运行任务的类并选择下一个任务。

### 2.1 RR 调度类（rr.c）

固定时间片轮转调度：

- **时间片**：10ms（`RR_TIME_SLICE_NS`）
- **队列**：FIFO 双向链表（`rq->runnable`）
- **调度逻辑**：
  1. 当前任务若仍在运行，移入队列尾部
  2. 取队列头部任务作为下一个运行任务
- **Enqueue**：任务加入队列尾部，`on_rq = 1`
- **Dequeue**：任务从队列移除，`on_rq = 0`

```
┌─────────┐     ┌────────┐     ┌────────┐     ┌────────┐
│  rq     │     │ task A │     │ task B │     │ task C │
│runnable │────▶│        │────▶│        │────▶│        │
│         │     │        │◀────│        │◀────│        │
└─────────┘     └────────┘     └────────┘     └────────┘
```

### 2.2 Idle 调度类

当没有可运行任务时，选择 idle 任务执行 `cpu_idle()` 进入低功耗等待。

## 3. 运行队列（struct rq）

每个 CPU 一个运行队列：

```c
struct rq {
    spinlock_t lock;
    struct task_struct *curr;     // 当前运行任务
    struct task_struct *idle;     // idle 任务
    struct list_head runnable;    // 可运行任务队列
    struct list_head tasks;       // 所有任务列表
    int nr_running;               // 可运行任务数
    int nr_tasks;                 // 总任务数
    struct timer sched_timer;     // 调度定时器（时间片到期）
};
```

## 4. 任务结构体（task_struct）

```c
struct task_struct {
    pid_t pid;                    // 进程 ID
    int status;                   // TASK_RUNNING / TASK_SLEEPING / TASK_ZOMBIE
    int prio;                     // 优先级
    int on_rq;                    // 是否在运行队列上
    int need_resched;             // 是否需要重调度

    struct sched_entity se;       // 调度实体
    const struct sched_class *sched_class;

    struct mm_struct *mm;         // 用户态内存描述符
    struct mm_struct *active_mm;  // 当前活动的页表（内核线程借用）

    struct thread_info *thread_info;
    struct list_head task_node;   // 全局任务链表节点
    struct list_head children;    // 子进程链表
    struct list_head sibling;     // 兄弟进程链表
    struct task_struct *parent;   // 父进程

    struct wait_queue_head wait;  // 等待队列
    struct wait_queue_head wait_child; // 子进程等待队列
    struct timer sleep_timer;     // 睡眠定时器

    spinlock_t lock;
};
```

## 5. 进程生命周期

### 5.1 fork

```c
pid_t fork(void);
```

复制当前进程：
1. 分配 `task_struct` 和内核栈
2. 复制父进程的 `mm_struct`（写时拷贝待实现）
3. 初始化调度实体（继承父进程时间片）
4. 设置状态为 `TASK_SLEEPING`，等待唤醒
5. 返回子进程 PID

### 5.2 execve

```c
int execve(const char *path, char *const argv[], char *const envp[]);
```

加载 ELF 可执行文件替换当前进程地址空间：
1. 解析 ELF header 和 program header
2. 通过 VMA 建立用户态虚拟地址映射（lazy map）
3. 设置入口点、栈指针
4. 返回用户态后缺页异常按需加载页面

### 5.3 exit

```c
void exit(int status);
```

1. 关闭所有打开的文件描述符
2. 设置状态为 `TASK_ZOMBIE`
3. 通知父进程（通过 wait_child 等待队列唤醒）
4. 从运行队列移除

### 5.4 waitpid

```c
pid_t waitpid(pid_t pid, int *wstatus, int options);
```

阻塞等待子进程退出：
1. 遍历子进程链表
2. 找到匹配的 zombie 子进程，回收并返回 PID
3. 若无子进程退出，当前进程进入 `TASK_SLEEPING` 等待

## 6. 上下文切换

```c
void sched(void)
```

1. 关中断 + 获取 rq 锁
2. `pick_next_task()` 选择下一个任务
3. 更新时间统计（`sum_exec_runtime`）
4. 切换页表（`pgtbl_switch_to()`）
5. 设置调度定时器（`timer_mod(&rq->sched_timer, now + time_slice)`）
6. `switch_to(prev, next)` — 架构相关的寄存器保存/恢复
7. `sched_tail()` — 更新前一个任务的时间统计

### 6.1 调度时机

- **时间片到期**：`sched_timer` 回调设置 `need_resched = 1`
- **主动让出**：`yield()` / `sleep_on()`
- **用户态返回**：`sched_handle_user_return()` 检查 `need_resched`
- **唤醒任务**：`wake_up_process()` 将任务加入运行队列

## 7. 等待队列（Wait Queue）

```c
struct wait_queue_head {
    struct list_head head;
};

void sleep_on(struct wait_queue_head *wq_head);   // 阻塞等待
void wake_up_one(struct wait_queue_head *wq_head); // 唤醒一个
void wake_up_all(struct wait_queue_head *wq_head); // 唤醒全部
```

用于实现阻塞 I/O、进程间同步等场景。

## 8. 内核线程

```c
pid_t kernel_thread(int (*fn)(void *), const char *name);
```

创建内核态线程，不拥有用户态地址空间（`mm = NULL`），切换时借用前一个进程的 `active_mm`。

启动时创建两个内核线程：
- **kernel_init**：初始化设备、挂载根文件系统、启动用户态 init
- **kthreadd**：内核线程守护进程（用于后续扩展）

## 9. MM 借用机制

内核线程没有用户态地址空间（`mm = NULL`），在 `sched_switch_mm()` 中借用前一个进程的 `active_mm`：

```c
if (next->mm == NULL) {
    next->active_mm = prev->active_mm;  // 借用
} else {
    next->active_mm = next->mm;
    pgtbl_switch_to(next->active_mm->pgdir);
    pgtbl_flush();
}
```

这确保内核线程切换时页表仍然有效，同时避免了不必要的 TLB 刷新。
