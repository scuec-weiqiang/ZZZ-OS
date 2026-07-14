#include <os/printk.h>
#include <os/kmalloc.h>
#include <os/wait.h>
#include <os/errno.h>
#include <os/sched.h>
#include <os/list.h>
#include <asm/ptrace.h>
#include <os/uaccess.h>

static int wait_find_child_locked(pid_t pid, struct task_struct **childp)
{
    int found_child = 0;
    struct task_struct *child;

    list_for_each_entry(child, &current->children, sibling) {
        if (pid > 0 && child->pid != pid) {
            continue;
        }

        found_child = 1;
        if (child->status == TASK_ZOMBIE) {
            *childp = child;
            return 1;
        }
    }

    *childp = NULL;
    return found_child ? 0 : -ECHILD;
}

static void wait_sleep_on_child(void)
{
    struct task_struct *current_task = current;
    struct wait_queue_head *wq_head = &current_task->wait_child;
    struct rq *rq;
    unsigned long rq_flags;

    current_task->status = TASK_SLEEPING;

    rq = this_rq();
    rq_flags = spin_lock_irqsave(&rq->lock);
    current_task->sched_class->dequeue_task(rq, current_task);
    current_task->wait_reason = wq_head->wait_reason;
    spin_unlock_irqrestore(&rq->lock, rq_flags);

    current_task->wait.private = current_task;
    wait_queue_add(wq_head, &current_task->wait);
}

static int wait_signal_pending(void)
{
    return (current->signal_pending & ~current->signal_blocked) != 0;
}

int do_waitpid(pid_t pid, int *status, int options) {
    while (1) {
        struct task_struct *child;
        unsigned long child_flags;
        int found;

        child_flags = spin_lock_irqsave(&current->lock);
        found = wait_find_child_locked(pid, &child);

        if (found == 1) {
            int release;
            unsigned long flags;
            pid_t child_pid;

            if (status) {
                copy_to_user((void*)status, (void*)&child->exit_code, sizeof(int));
            }

            child_pid = child->pid;
            list_del(&child->sibling);
            child->parent = NULL;
            spin_unlock_irqrestore(&current->lock, child_flags);

            task_detach_from_rq(child);

            flags = spin_lock_irqsave(&child->lock);
            child->status = TASK_DEAD;
            release = !child->on_cpu;
            spin_unlock_irqrestore(&child->lock, flags);

            if (release) {
                task_destroy(child);
            }

            return child_pid;
        }

        spin_unlock_irqrestore(&current->lock, child_flags);

        if (found < 0) {
            return -ECHILD;
        }
        
        if (options & WNOHANG)
            return 0;

        if (wait_signal_pending()) {
            return -EINTR;
        }

        unsigned long wq_flags = spin_lock_irqsave(&current->wait_child.lock);
        child_flags = spin_lock_irqsave(&current->lock);
        found = wait_find_child_locked(pid, &child);
        spin_unlock_irqrestore(&current->lock, child_flags);
        if (found == 0) {
            wait_sleep_on_child();
            spin_unlock_irqrestore(&current->wait_child.lock, wq_flags);
            sched();
            wq_flags = spin_lock_irqsave(&current->wait_child.lock);
            if (!list_empty(&current->wait.list)) {
                wait_queue_remove(&current->wait_child, &current->wait);
            }
            spin_unlock_irqrestore(&current->wait_child.lock, wq_flags);
            if (wait_signal_pending()) {
                return -EINTR;
            }
        } else {
            spin_unlock_irqrestore(&current->wait_child.lock, wq_flags);
        }
    }
}

int do_wait(int *status) {
    return do_waitpid(-1, status, 0);
}

__SYSCALL__ long sys_waitpid(struct pt_regs *ctx) {
    pid_t pid = (pid_t)ctx->r[0];
    int *status = (int *)ctx->r[1];
    int options = (int)ctx->r[2];
    return do_waitpid(pid, status, options);
}

__SYSCALL__ long sys_wait4(struct pt_regs *ctx) {
    return sys_waitpid(ctx);
}
