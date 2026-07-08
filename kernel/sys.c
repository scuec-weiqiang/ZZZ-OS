#include <os/cpu.h>
#include <os/errno.h>
#include <os/sched.h>
#include <os/signal.h>
#include <os/syscall.h>

static struct signal_struct *task_signal(struct task_struct *task) {
    return task ? task->signal : NULL;
}

static struct task_struct *sys_task_by_pid(pid_t pid) {
    if (pid == 0) {
        return current;
    }

    return find_task_by_pid(pid);
}

int task_pgrp_exists(pid_t pgid) {
    int cpu_num;

    if (pgid <= 0 || global_rq == NULL) {
        return 0;
    }

    cpu_num = smp_get_cpu_count();
    if (cpu_num > MAX_CPUS) {
        cpu_num = MAX_CPUS;
    }

    for (int cpu = 0; cpu < cpu_num; cpu++) {
        struct rq *rq = &global_rq[cpu];
        struct task_struct *task;
        unsigned long flags;

        flags = spin_lock_irqsave(&rq->lock);
        list_for_each_entry(task, &rq->tasks, struct task_struct, task_node) {
            if (task->signal != NULL && task->signal->pgrp == pgid) {
                spin_unlock_irqrestore(&rq->lock, flags);
                return 1;
            }
        }
        spin_unlock_irqrestore(&rq->lock, flags);
    }

    return 0;
}

int send_signal_to_pgrp(pid_t pgid, int sig) {
    int cpu_num;
    int sent = 0;

    if (pgid <= 0 || sig < 0 || sig >= NSIG || global_rq == NULL) {
        return -EINVAL;
    }

    cpu_num = smp_get_cpu_count();
    if (cpu_num > MAX_CPUS) {
        cpu_num = MAX_CPUS;
    }

    for (int cpu = 0; cpu < cpu_num; cpu++) {
        struct rq *rq = &global_rq[cpu];
        struct task_struct *task, *tmp;
        unsigned long flags;

        flags = spin_lock_irqsave(&rq->lock);
        list_for_each_entry_safe(task, tmp, &rq->tasks, struct task_struct, task_node) {
            if (task->signal != NULL && task->signal->pgrp == pgid) {
                task->signal_pending |= 1UL << sig;
                sent++;
                spin_unlock_irqrestore(&rq->lock, flags);
                wake_up_process(task);
                flags = spin_lock_irqsave(&rq->lock);
            }
        }
        spin_unlock_irqrestore(&rq->lock, flags);
    }

    return sent ? 0 : -ESRCH;
}

SYSCALL_DEFINE1(getpgid, pid_t, pid) {
    struct task_struct *task = sys_task_by_pid(pid);
    struct signal_struct *sig = task_signal(task);

    if (sig == NULL) {
        return -ESRCH;
    }

    return sig->pgrp;
}

SYSCALL_DEFINE0(getpgrp) {
    if (current == NULL || current->signal == NULL) {
        return -ESRCH;
    }

    return current->signal->pgrp;
}

SYSCALL_DEFINE2(setpgid, pid_t, pid, pid_t, pgid) {
    struct task_struct *task = sys_task_by_pid(pid);
    struct signal_struct *sig;

    if (task == NULL || task->signal == NULL || current == NULL || current->signal == NULL) {
        return -ESRCH;
    }

    if (pgid < 0) {
        return -EINVAL;
    }
    if (pgid == 0) {
        pgid = task->pid;
    }

    sig = task->signal;
    if (sig->session != current->signal->session) {
        return -EPERM;
    }

    if (pgid != task->pid && !task_pgrp_exists(pgid)) {
        return -EPERM;
    }

    sig->pgrp = pgid;
    return 0;
}

SYSCALL_DEFINE1(getsid, pid_t, pid) {
    struct task_struct *task = sys_task_by_pid(pid);
    struct signal_struct *sig = task_signal(task);

    if (sig == NULL) {
        return -ESRCH;
    }

    return sig->session;
}

SYSCALL_DEFINE0(setsid) {
    if (current == NULL || current->signal == NULL) {
        return -ESRCH;
    }

    if (current->signal->pgrp == current->pid) {
        return -EPERM;
    }

    current->signal->session = current->pid;
    current->signal->pgrp = current->pid;
    current->signal->tty = NULL;
    current->signal->tty_old_pgrp = 0;
    return current->pid;
}
