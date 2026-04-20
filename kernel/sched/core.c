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
#include <mm/pgtbl.h>
#include <asm/switch_to.h>

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

static void task_attach_to_rq(struct task_struct *task) {
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

static void task_detach_from_rq(struct task_struct *task) {
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
 * Perform scheduler related setup for a newly forked process p.
 * p is forked by current.
 *
 * __sched_fork() is basic setup used by init_idle() too:
 */
static void __sched_fork(struct task_struct *p) {
	p->on_rq			= 0;
	p->se.exec_start		= 0;
	p->se.sum_exec_runtime		= 0;
	p->se.time_slice			= NSEC_PER_SEC;
	INIT_LIST_HEAD(&p->se.sched_node);
}

int sched_fork(struct task_struct *p) {
	unsigned long flags;
	int cpu = get_cpuid();

	__sched_fork(p);
	/*
	 * We mark the process as running here. This guarantees that
	 * nobody will actually run it, and a signal or other external
	 * event cannot wake it up and insert it on the runqueue either.
	 */
	p->status = TASK_RUNNING;
    INIT_LIST_HEAD(&p->task_node);
	/*
	 * Make sure we do not leak PI boosting priority to the child.
	 */
	p->prio = current->normal_prio;

	p->sched_class = &rr_sched_class;

	flags = spin_lock_irqsave(&p->lock);
    task_thread_info(p)->cpu = cpu;
    task_attach_to_rq(p);
	spin_unlock_irqrestore(&p->lock, flags);

	return 0;
}

static void sched_switch_mm(struct task_struct *next) {
    struct mm_struct *next_mm;

    if (next != NULL && next->active_mm != NULL) {
        next_mm = next->active_mm;
    } else {
        next_mm = this_rq()->curr->active_mm;
    }

    if (this_rq()->curr->active_mm != next_mm) {
        pgtbl_switch_to(next_mm->pgdir);
        pgtbl_flush();
    }
    
}

static void sched_finish_prev(struct rq *rq, struct task_struct *prev) {
    if (rq == NULL || prev == NULL) {
        return;
    }

    if (prev->status == TASK_ZOMBIE || prev->status == TASK_DEAD) {
        // dprintk("task pid=%xu exit with code %d, clean up\n", (unsigned long)prev->pid, prev->exit_code);
        task_detach_from_rq(prev);
        task_destroy(prev);
    }
}

void sched_tail(struct task_struct *prev) {
    struct rq *rq = this_rq();
    if (rq == NULL || prev == NULL) {
        return;
    }
    preempt_disable();
    if (prev->status == TASK_ZOMBIE || prev->status == TASK_DEAD) {
        sched_finish_prev(rq, prev);
    }
    preempt_enable(); 
    local_irq_enable();
}

void __sched sched(void) {
    struct rq *rq = this_rq();
    struct task_struct *next = NULL;
    struct task_struct *prev = NULL;
    struct task_struct *last = NULL;
    unsigned long flags;

    // 会在sched_finish_prev中打开
    local_irq_disable();
    flags = spin_lock_irqsave(&rq->lock);
    next = pick_next_task(rq);

    next->status = TASK_RUNNING;    
    rq->sched_timer.expires_ns = monotonic_ns() + next->se.time_slice;
    prev = rq->curr;
    rq->curr = next;
    timer_start(&rq->sched_timer);
    
    spin_unlock_irqrestore(&rq->lock, flags);
    sched_switch_mm(next);
    printk(BLUE("switch from pid=%du to pid=%du\n"), prev->pid, next->pid);
    switch_to(prev, next, last);
    sched_tail(last);
}

void sched_event(struct timer *t, void *arg) {
    this_rq()->curr->need_resched = 1;
}

void yield() {
    sched();
}

void sleep_on(struct wait_queue_head *wq_head) {
    struct task_struct *current_task = this_rq()->curr;
    if (current_task->status != TASK_RUNNING) {
        return;
    }
    current_task->status = TASK_SLEEPING;
    // 从当前 CPU 的运行队列中移除当前任务，放入等待队列
    current_task->sched_class->dequeue_task(this_rq(), current_task);
    current_task->wait.private = current_task;
    wait_queue_add(wq_head, &current_task->wait);
    
    sched();
}

void wake_up_one(struct wait_queue_head *wq_head) {
    struct wait_queue *wq, *tmp;
    list_for_each_entry_safe(wq, tmp, &wq_head->head, struct wait_queue, list) {
        struct task_struct *task = wq->private;
        if (task->status == TASK_SLEEPING) {
            task->status = TASK_RUNNING;
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
            task->status = TASK_RUNNING;
            wait_queue_remove(wq_head, wq);
            wake_up_process(task);
        }
    }
}

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
