#include <os/sched.h>
#include <os/printk.h>
#include <os/cpu.h>
#include <asm/syscall_num.h>
#include <os/list.h>
#include <os/signal.h>
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
    unsigned long parent_flags;

    parent_flags = spin_lock_irqsave(&parent->lock);
    list_for_each_entry_safe(child, n, &parent->children, struct task_struct, sibling) {
        unsigned long reaper_flags;

        list_del(&child->sibling);
        // dprintk("reparent child pid=%d to init\n", child->pid);
        reaper = choose_reaper(child);
        reaper_flags = spin_lock_irqsave(&reaper->lock);
        child->parent = reaper;
        list_add_tail(&reaper->children, &child->sibling);
        spin_unlock_irqrestore(&reaper->lock, reaper_flags);

        if (child->status == TASK_ZOMBIE)
            wake_up_one(&reaper->wait_child);
    }
    spin_unlock_irqrestore(&parent->lock, parent_flags);
}

long sys_exit(struct pt_regs *ctx)
{
    do_exit((int)ctx->r[0]);
    return 0;
}

void __noreturn do_exit(int code) {
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
    curr->sched_class->dequeue_task(rq, curr);

    spin_unlock_irqrestore(&rq->lock, flags);

    #ifdef SYS_TRACE_ENABLE
    printk("[exit] pid=%d exit with code %d\n", current->pid, code);
    #endif
    
    reparent_children(curr);

    if (curr->parent) {
        send_signal(curr->parent, SIGCHLD);
        wake_up_one(&curr->parent->wait_child);
    }
    sched();
     __builtin_unreachable();
}
