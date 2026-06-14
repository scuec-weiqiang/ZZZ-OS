#include <os/check.h>
#include <os/cpu.h>
#include <os/irq.h>
#include <os/kmalloc.h>
#include <os/mm.h>
#include <os/of_cpu.h>
#include <os/cpu.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/timekeeping.h>
#include <os/preempt.h>
#include <os/syscall_num.h>
#include <mm/pgtbl.h>
#include <asm/switch_to.h>
#include <asm/ptrace.h>

struct rq *global_rq;

struct rq *this_rq(void) {
    CHECK(global_rq != NULL, "scheduler: runqueue is not initialized", return NULL;);
    return &global_rq[get_cpuid()];
}

static const struct sched_class *sched_class_highest(void) {
    return &rr_sched_class;
}

static struct task_struct *pick_next_task(struct rq *rq) {
    const struct sched_class *class;

    CHECK(rq != NULL, "scheduler: invalid runqueue", return NULL;);

    for (class = sched_class_highest(); class != NULL; class = class->next) {
        struct task_struct *p;
        if (class->pick_next_task == NULL) {
            continue;
        }

        p = class->pick_next_task(rq);
        if (p != NULL) {
            return p;
        }
    }

    return NULL;
}

void task_attach_to_rq(struct task_struct *task) {
    struct rq *rq;
    struct thread_info *ti = task_thread_info(task);
    unsigned long flags;
    if (task == NULL || global_rq == NULL) {
        
        return;
    }
    
    rq = &global_rq[ti->cpu];
    flags = spin_lock_irqsave(&rq->lock);
    if (list_empty(&task->task_node)) {
        list_add_tail(&rq->tasks, &task->task_node);
        rq->nr_tasks++;
    }
    spin_unlock_irqrestore(&rq->lock, flags);
}

void task_detach_from_rq(struct task_struct *task) {
    struct rq *rq;
    struct thread_info *ti = task_thread_info(task);
    unsigned long flags;

    if (task == NULL || global_rq == NULL) {
        return;
    }

    rq = &global_rq[ti->cpu];
    flags = spin_lock_irqsave(&rq->lock);
    if (!list_empty(&task->task_node)) {
        list_del(&task->task_node);
        if (rq->nr_tasks > -1) {
            rq->nr_tasks--;
        }
    }

    spin_unlock_irqrestore(&rq->lock, flags);
}
#define RR_TIME_SLICE_NS 10000000 // 10ms

/*
    设置调度器相关的字段
*/
static void __sched_fork(struct task_struct *p) {
	p->on_rq			= 0;
	p->se.exec_start		= monotonic_ns();
	p->se.sum_exec_runtime		= 0;
	p->se.time_slice			= current->se.time_slice;
	INIT_LIST_HEAD(&p->se.sched_node);
}

/* 复制并初始化task的调度器 */
void sched_fork(struct task_struct *p) {
	unsigned long flags;
	int cpu = get_cpuid();

	__sched_fork(p);

	p->status = TASK_SLEEPING;
    p->prio = current->base_prio;
    p->base_prio = current->base_prio;
	p->sched_class = &rr_sched_class;

    INIT_LIST_HEAD(&p->task_node);
    INIT_LIST_HEAD(&p->wait.list);
    INIT_LIST_HEAD(&p->wait_child.head);
    p->wait.private = p;

	flags = spin_lock_irqsave(&p->lock);

    task_thread_info(p)->cpu = cpu;

	spin_unlock_irqrestore(&p->lock, flags);
}

static void sched_switch_mm(struct task_struct *prev, struct task_struct *next) {
    if (next->mm == NULL) {
        // 考虑线程没有属于自己的地址空间，那就借用上一个进程的地址空间
        // printk("borrow mm of pid=%d for pid=%d\n", prev->pid, next->pid);
        next->active_mm = prev->active_mm;
    } else {
        // 有的话就直接切页表
        next->active_mm = next->mm;
        pgtbl_switch_to(next->active_mm->pgdir);
        pgtbl_flush();
    }
}


void sched_tail(struct task_struct *prev) {
    prev->se.sum_exec_runtime += monotonic_ns() - prev->se.exec_start;
}

void sched_handle_user_return(void)
{
    if (current != NULL && current->need_resched) {
        current->need_resched = 0;
        sched();
    }
}

void __sched sched(void) {
    struct rq *rq = this_rq();
    struct task_struct *next = NULL;
    struct task_struct *prev = NULL;
    struct task_struct *last = NULL;
    unsigned long flags;

    local_irq_disable();

    flags = spin_lock_irqsave(&rq->lock);
    next = pick_next_task(rq);

    u64 now = monotonic_ns();
    
    next->se.exec_start =  now;
    prev = rq->curr;
    rq->curr = next;
    spin_unlock_irqrestore(&rq->lock, flags);
    timer_mod(&rq->sched_timer, now + next->se.time_slice);
    sched_switch_mm(prev, next);

    // printk(BLUE("switch from pid=%du to pid=%du\n"), prev->pid, next->pid);

    switch_to(prev, next, last);
    sched_tail(last);
}

void sched_event(struct timer *t, void *arg) {
    struct rq *rq = this_rq();
    const struct sched_class *class;

    // 1. 通知当前任务的调度类：时间片用完（只计数，不操作队列）
    if (rq->curr->sched_class && rq->curr->sched_class->task_tick)
        rq->curr->sched_class->task_tick(rq, rq->curr);

    // 2. 遍历所有调度类，各调度类独立执行老化检查（低优先级 → 提权）
    for (class = sched_class_highest(); class; class = class->next) {
        if (class->aging)
            class->aging(rq);
    }

    rq->curr->need_resched = 1;
}

void yield() {
    sched();
}

long sys_getpid(struct pt_regs *ctx)
{
    (void)ctx;
    return current->pid;
}

void sleep_on(struct wait_queue_head *wq_head) {
    // struct task_struct *current_task = this_rq()->curr;
    struct task_struct *current_task = current;
    if (current_task->status != TASK_RUNNING) {
        return;
    }
    if (current_task->status == TASK_SLEEPING) {
        return;
    }
    current_task->status = TASK_SLEEPING;

    int flags = spin_lock_irqsave(&current_task->lock);

    // 从当前 CPU 的运行队列中移除当前任务，放入等待队列
    current_task->sched_class->dequeue_task(this_rq(), current_task);
    current_task->wait.private = current_task;
    wait_queue_add(wq_head, &current_task->wait);

    spin_unlock_irqrestore(&current_task->lock, flags);
    // dprintk("sleep task:%d\n",current_task->pid);
    sched();
}

void wake_up_one(struct wait_queue_head *wq_head) {
    struct wait_queue *wq, *tmp;
    list_for_each_entry_safe(wq, tmp, &wq_head->head, struct wait_queue, list) {
        struct task_struct *task = wq->private;
        if (task->status == TASK_SLEEPING) {
            // dprintk("wake up task:%d\n",task->pid);
            wait_queue_remove(wq_head, wq);
            wake_up_process(task);
            break; // 只唤醒一个
        }
    }
}

void wake_up_all(struct wait_queue_head *wq_head) {
    struct wait_queue *wq, *tmp;
    list_for_each_entry_safe(wq, tmp, &wq_head->head, struct wait_queue, list) {
        struct task_struct *task = wq->private;
        if (task->status == TASK_SLEEPING) {
            wait_queue_remove(wq_head, wq);
            wake_up_process(task);
        }
    }
}
// sk-c5765c5aa4e14cb19aefca18c7067d05

/*
 * 为非 0 号 CPU 创建 idle task
 */
static struct task_struct *create_idle_task(int cpu) {
    struct task_struct *idle;
    struct thread_info *ti;
    void *stack;

    /* 分配 task_struct */
    idle = (struct task_struct *)kmalloc(sizeof(struct task_struct));
    if (!idle) {
        printk("sched: failed to alloc idle task for CPU %d\n", cpu);
        return NULL;
    }

    /* 分配内核栈 */
    stack = kmalloc(THREAD_SIZE);
    if (!stack) {
        printk("sched: failed to alloc idle stack for CPU %d\n", cpu);
        kfree(idle);
        return NULL;
    }

    /* 从 init_task 复制基础字段 */
    memcpy(idle, &init_task, sizeof(struct task_struct));

    /* thread_info 位于栈底 */
    ti = (struct thread_info *)stack;
    memset(ti, 0, sizeof(struct thread_info));
    ti->task = idle;
    ti->cpu  = cpu;

    idle->stack = stack;
    idle->pid   = 0;       /* idle task pid = 0 */
    idle->sched_class = &idle_sched_class;

    // 初始化链表节点 
    INIT_LIST_HEAD(&idle->se.sched_node);
    INIT_LIST_HEAD(&idle->task_node);
    INIT_LIST_HEAD(&idle->children);
    INIT_LIST_HEAD(&idle->sibling);
    INIT_LIST_HEAD(&idle->wait.list);
    INIT_LIST_HEAD(&idle->wait_child.head);
    idle->wait.private = idle;

    spin_lock_init(&idle->lock);

    extern struct secondary_data secondary_data[MAX_CPUS];
    // 回填辅助核 release 数据的栈指针 (结构定义在 os/cpu.h) 
    secondary_data[cpu].stack = (void *)((unsigned long)stack + THREAD_SIZE);

    return idle;
}

void sched_init(void) {
    int cpu_num = of_get_cpu_num();
    CHECK(cpu_num > 0, "scheduler: invalid cpu count", return;);

    global_rq = (struct rq *)kmalloc((size_t)cpu_num * sizeof(*global_rq));
    CHECK(global_rq != NULL, "scheduler: alloc runqueue failed", return;);

    global_rq[0].idle = setup_init_task();
    global_rq[0].curr = global_rq[0].idle;

    for (int cpu = 0; cpu < cpu_num; cpu++) {
        struct rq *rq = &global_rq[cpu];
        rq->sched_timer.cpu = cpu;
        rq->sched_timer.active = false;
        rq->sched_timer.expires_ns = UINT64_MAX - 1;
        rq->sched_timer.period_ns = 0; 
        rq->sched_timer.arg = NULL;
        rq->sched_timer.pinned = cpu;
        rq->sched_timer.callback = sched_event;
        spin_lock_init(&rq->lock);
        for (int prio = PRIO_HIGHEST; prio <= PRIO_LOWEST; prio ++) {
                INIT_LIST_HEAD(&rq->runnable[prio]);
        }
        rq->prio_bitmap = 0;
        for (int prio = 0; prio < PRIO_NUMS; prio++) {
            rq->prio_last_served[prio] = 0;
        }
        INIT_LIST_HEAD(&rq->tasks);
        rq->nr_running = 0;
        rq->nr_tasks = 0;
    }

    // 为其他 CPU 创建 idle task
    for (int cpu = 1; cpu < cpu_num; cpu++) {
        struct task_struct *idle = create_idle_task(cpu);
        if (idle) {
            global_rq[cpu].idle = idle;
            printk("sched: idle task for CPU %d created (stack=%p)\n", cpu, idle->stack);
        }
    }

    sched();
}
