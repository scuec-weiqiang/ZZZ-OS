#include <os/sched.h>
#include <os/printk.h>
#include <os/cpu.h>

void __noreturn do_exit(int code)
{
    struct rq *rq = this_rq();
    struct task_struct *curr = current;
    unsigned long flags;

    flags = spin_lock_irqsave(&rq->lock);

    curr->exit_code = code;
    curr->status = TASK_ZOMBIE;

    /* 唤醒父进程/等待者 */
    /* cancel 当前 rq 的调度 timer，如有需要 */

    spin_unlock_irqrestore(&rq->lock, flags);
    dprintk("task pid=%xu exit with code %d, hand off to scheduler for cleanup\n", (unsigned long)curr->pid, code);
    curr->sched_class->dequeue_task(rq, curr);
    sched();
}
