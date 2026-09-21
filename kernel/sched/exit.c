#include <os/sched.h>
#include <os/printk.h>
#include <os/cpu.h>
#include <asm/syscall_num.h>
#include <os/list.h>
#include <os/signal.h>
#include <os/uaccess.h>
#include <asm/ptrace.h>
#include <os/completion.h>
#include <os/err.h>
#include <mm/pgtbl.h>
#include <mm/vma.h>
#include <os/kva.h>
#include <os/mm.h>
extern struct task_struct init_task;

static void clear_child_tid_user(struct task_struct *task)
{
    struct mm_struct *mm;
    int __user *uaddr;
    int zero = 0;
    unsigned char *src = (unsigned char *)&zero;
    unsigned long flags;

    if (task == NULL || task->clear_child_tid == NULL) {
        return;
    }

    mm = task->mm;
    uaddr = task->clear_child_tid;
    task->clear_child_tid = NULL;

    if (mm == NULL || mm->pgdir == NULL) {
        return;
    }

    spin_lock_irqsave(&mm->lock, &flags);
    for (size_t i = 0; i < sizeof(zero); i++) {
        virt_addr_t va = (virt_addr_t)uaddr + i;
        struct vma *vma = vma_find(mm, va);
        phys_addr_t pa;

        if (IS_ERR(vma) || !(vma->flags & PROT_USER) ||
            !(vma->flags & PROT_WRITE)) {
            goto out_unlock;
        }

        pa = pgtbl_lookup(mm->pgdir, va);
        if (pa == 0) {
            goto out_unlock;
        }

        *(unsigned char *)KERNEL_VA(pa) = src[i];
    }

out_unlock:
    spin_unlock_irqrestore(&mm->lock, flags);
}

struct task_struct *choose_reaper(struct task_struct *child) {
    if (child->flags & PF_KTHREAD)
        return kthreadd_task;   // 内核进程交给 PID 2回收
    else
        return find_task_by_pid(1); // 用户态进程交给 PID 1 init
}

void reparent_children(struct task_struct *parent) {
    struct task_struct *child, *n, *reaper;
    unsigned long parent_flags;

    spin_lock_irqsave(&parent->lock, &parent_flags);
    list_for_each_entry_safe(child, n, &parent->children, sibling) {
        list_del(&child->sibling);
        // dprintk("reparent child pid=%d to init\n", child->pid);
        reaper = choose_reaper(child);
        // spin_lock_irqsave(&reaper->lock, &reaper_flags);
        child->parent = reaper;
        list_add_tail(&reaper->children, &child->sibling);
        // spin_unlock_irqrestore(&reaper->lock, reaper_flags);

        if (child->status == TASK_ZOMBIE)
            wake_up_one(&reaper->wait_child);
    }
    spin_unlock_irqrestore(&parent->lock, parent_flags);
}

__SYSCALL__ long sys_exit(struct pt_regs *ctx)
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

    spin_lock_irqsave(&rq->lock, &flags);

    curr->exit_code = code;
    curr->status = TASK_ZOMBIE;
    curr->sched_class->dequeue_task(rq, curr);

    spin_unlock_irqrestore(&rq->lock, flags);

    clear_child_tid_user(curr);

    #ifdef SYS_TRACE_ENABLE
    printk("[exit] pid=%d exit with code %d\n", current->pid, code);
    #endif

    reparent_children(curr);

    if (curr->vfork_done) {
        complete(curr->vfork_done);
        curr->vfork_done = NULL;
    }

    struct task_struct *parent = curr->parent;
    if (parent) {
        send_signal(parent, SIGCHLD);
        wake_up_one(&parent->wait_child);
    }

    sched();
    __builtin_unreachable();
}

__SYSCALL__ long sys_exit_group(struct pt_regs *ctx)
{
    return sys_exit(ctx);
}
