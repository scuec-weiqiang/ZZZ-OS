#include <os/check.h>
#include <os/cpu.h>
#include <os/irq.h>
#include <os/kmalloc.h>
#include <os/mm.h>
#include <os/of_cpu.h>
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

// static inline unsigned long sched_read_cpsr(void)
// {
//     unsigned long cpsr;
//     asm volatile("mrs %0, cpsr" : "=r"(cpsr));
//     return cpsr;
// }

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
        // if (class == &rr_sched_class) {
        //     dprintk("pick_next_task: check rr class, rq->nr_running=%d\n", rq->nr_running);
        // } else if (class == &idle_sched_class) {
        //     dprintk("pick_next_task: check idle class\n");
        // }

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
    p->prio = current->prio;
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
    this_rq()->curr->need_resched = 1;
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
void sched_init(void) {
    int cpu_num = of_get_cpu_num();
    CHECK(cpu_num > 0, "scheduler: invalid cpu count", return;);
    
    global_rq = (struct rq *)kmalloc((size_t)cpu_num * sizeof(*global_rq));
    CHECK(global_rq != NULL, "scheduler: alloc runqueue failed", return;);

    global_rq[0].idle = setup_init_task();
    global_rq[0].curr = global_rq[0].idle;

    int cpuid = get_cpuid();
    for (int cpu = 0; cpu < cpu_num; cpu++) {
        struct rq *rq = &global_rq[cpu];
        rq->sched_timer.cpu = cpuid;
        rq->sched_timer.active = false;
        rq->sched_timer.expires_ns = UINT64_MAX - 1;
        rq->sched_timer.period_ns = 0; // one shot
        rq->sched_timer.arg = NULL;
        rq->sched_timer.pinned = cpuid;
        rq->sched_timer.callback = sched_event;
        spin_lock_init(&rq->lock);
        INIT_LIST_HEAD(&rq->runnable);
        INIT_LIST_HEAD(&rq->tasks);
        rq->nr_running = 0;
        rq->nr_tasks = 0;
    }

    sched();
}
