#include <os/sched.h>
#include <os/printk.h>
#include <os/cpu.h>
#include <os/syscall_num.h>
#include <os/list.h>
#include <asm/ptrace.h>
extern struct task_struct init_task;

struct task_struct *choose_reaper(struct task_struct *child) {
    if (child->flags & PF_KTHREAD)
        return kthreadd_task;   // PID 2
    else
        return find_task_by_pid(1);       // PID 1
}

void reparent_children(struct task_struct *parent) {
    struct task_struct *child, *n, *reaper;
    list_for_each_entry_safe(child, n, &parent->children, struct task_struct, sibling) {
        list_del(&child->sibling);
        // dprintk("reparent child pid=%d to init\n", child->pid);
        reaper = choose_reaper(child);
        child->parent = reaper;
        list_add_tail(&reaper->children, &child->sibling);

        if (child->status == TASK_ZOMBIE)
            wake_up_one(&reaper->wait_child);
    }
}

long sys_exit(struct pt_regs *ctx)
{
    do_exit((int)ctx->r[0]);
    return 0;
}

void __noreturn do_exit(int code)
{
    struct rq *rq = this_rq();
    struct task_struct *curr = current;
    unsigned long flags;
    
    // dprintk("mode=%xu sp=%xu current=%d rq_curr=%d exit_code=%d\n",
    //         cpsr & MODE_MASK, current_stack_pointer,
    //         curr ? curr->pid : -1,
    //         (rq && rq->curr) ? rq->curr->pid : -1,
    //         code);

    flags = spin_lock_irqsave(&rq->lock);

    curr->exit_code = code;
    curr->status = TASK_ZOMBIE;

    /* 唤醒父进程/等待者 */
    /* cancel 当前 rq 的调度 timer，如有需要 */

    spin_unlock_irqrestore(&rq->lock, flags);
    printk("pid=%xu exit with code %d\n", (unsigned long)curr->pid, code);

    reparent_children(curr);

    curr->sched_class->dequeue_task(rq, curr);

    if (curr->parent) {
        wake_up_one(&curr->parent->wait_child);
    }
    sched();
     __builtin_unreachable();
}
