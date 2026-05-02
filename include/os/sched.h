/**
 * @FilePath: /ZZZ-OS/kernel/os/sched.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-01 02:29:14
 * @LastEditTime: 2025-10-06 18:53:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef __OS_SCHED_H
#define __OS_SCHED_H

#include <os/list.h>
#include <os/spinlock.h>
#include <os/timerqueue.h>
#include <os/pfn.h>
#include <os/wait.h>
#include <asm/thread_info.h>

#define PROC_DEFAULT_SLICE 500

#define PF_KTHREAD	0x00200000	/* I am a kernel thread */

/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New mount namespace group */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
/* 0x02000000 was previously the unused CLONE_STOPPED (Start in stopped state)
   and is now available for re-use. */
#define CLONE_NEWUTS		0x04000000	/* New utsname namespace */
#define CLONE_NEWIPC		0x08000000	/* New ipc namespace */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */

typedef int pid_t;
struct rq;
struct task_struct;
struct elf_info;
struct files_struct;
struct mm_struct;
struct fs_struct;

enum task_status {
    TASK_RUNNING,     // 正在运行
    TASK_SLEEPING,   
    TASK_ZOMBIE,      // 退出未回收
};

struct sched_entity {
    struct list_head sched_node;
    u64 exec_start; // 开始执行的时间
    union {
        u64 time_slice; // 任务时间片
        u64 vruntime; // 虚拟运行时
    };
    u64 sum_exec_runtime; // 总共运行了多久
};

struct sched_class {
    const char *name;
    const struct sched_class *next;

    void (*enqueue_task)(struct rq *rq, struct task_struct *p);
    void (*dequeue_task)(struct rq *rq, struct task_struct *p);
    struct task_struct *(*pick_next_task)(struct rq *rq);
};

extern struct sched_class rr_sched_class;
extern struct sched_class idle_sched_class;

#define task_thread_info(task)	((struct thread_info *)(task)->stack)
#define task_stack_page(task)	((task)->stack)

union thread_union {
	struct thread_info thread_info;
	unsigned long stack[THREAD_SIZE/sizeof(long)];
};

struct task_struct {
    pid_t pid;            //进程ID
    // pid_t tgid;
    unsigned int flags;
    void *stack;      // 内核栈基址
    enum task_status status;         //进程状态
    int in_execve;        // 是否正在执行 execve 系统调用

    int on_rq;            // 是否在 runnable 队列中
    int exit_code;
    int prio;

    spinlock_t lock;

    struct list_head task_node; // 挂在 rq->tasks 上，跟踪该 CPU 上所有任务
    struct sched_entity se; 
    struct sched_class *sched_class;

    struct files_struct *files; //文件描述符表
    struct mm_struct *mm; //进程内存管理结构
    struct mm_struct *active_mm; // 进程当前使用的内存管理结构，可能是自己的mm，也可能是父进程的mm（vfork）
    struct fs_struct *fs; //进程文件系统信息结构
    
    struct wait_queue wait; // 进程等待队列头
    int need_resched;

    struct task_struct *parent;
    struct list_head children;
    struct list_head sibling;
    struct wait_queue_head wait_child; 

    struct timer sleep_timer; // 睡眠定时器
};
extern struct task_struct* find_task_by_pid(pid_t pid);
extern struct task_struct *kthreadd_task;

static inline void setup_thread_stack(struct task_struct *p, struct task_struct *org) {
	*task_thread_info(p) = *task_thread_info(org);
	task_thread_info(p)->task = p;
}

static inline unsigned long *end_of_stack(struct task_struct *p) {
	return (unsigned long *)(task_thread_info(p) + 1);
}

#define task_pt_regs(p) \
	((struct pt_regs *)(THREAD_START_SP + task_stack_page(p)) - 1)

#define get_current() (current_thread_info()->task)
#define current get_current()

struct rq {
    spinlock_t lock;
    struct list_head runnable; // 可运行任务（不含curr）
    struct list_head tasks; // 所有任务，包括可运行，等待，睡眠，僵尸等

    struct task_struct *curr; // 当前正在运行的任务
    struct task_struct *idle;

    struct timer sched_timer;
    int nr_running;
    int nr_tasks;
};

extern void task_attach_to_rq(struct task_struct *task);
extern void task_detach_from_rq(struct task_struct *task);
extern void sleep_on(struct wait_queue_head *wq_head);
extern void wake_up_one(struct wait_queue_head *wq_head);
extern void wake_up_all(struct wait_queue_head *wq_head);
extern int do_waitpid(pid_t pid, int *status);
extern int do_wait(int *status);

#define __sched		__attribute__((__section__(".text.sched.")))

extern struct rq *this_rq(void);
extern void sched_init();
extern void sched();
extern void sched_handle_user_return(void);
extern void yield();
extern void sched_tick(void);
extern void sched_kthread_test(void);
extern void __noreturn do_exit(int code);
extern struct rq *global_rq;
extern void sched_fork(struct task_struct *p);
extern int kthreadd(void *arg);
extern pid_t kernel_thread(int (*fn)(void *), void *arg);
extern struct task_struct*  kthread_create(int (*fn)(void *), void *arg);
extern void task_destroy(struct task_struct *task);
extern void wake_up_process(struct task_struct *p);
extern struct task_struct* setup_init_task(void);

#endif
