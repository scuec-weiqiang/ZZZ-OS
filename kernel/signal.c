#include <os/types.h>
#include <os/sched.h>
#include <os/signal.h>
#include <os/err.h>
#include <asm/ptrace.h>

long sys_kill(struct pt_regs *ctx) {
    int pid = ctx->r[0];
    int sig = ctx->r[1];
    
    struct task_struct *t;

    if (sig < 0 || sig >= NSIG)
        return -EINVAL;

    t = find_task_by_pid(pid);
    if (!t)
        return -ESRCH;

    if (sig == 0)
        return 0;

    t->signal_pending |= 1UL << sig;
    wake_up_process(t);
    return 0;
}

void handle_pending_signal(struct pt_regs *regs) {
    unsigned long pending = current->signal_pending & ~current->signal_blocked;

    if (!pending)
        return;

    if (pending & (1UL << SIGKILL)) {
        do_exit(128 + SIGKILL);
    }
        

    if (pending & (1UL << SIGTERM)){
        do_exit(128 + SIGTERM);
    }

    // if (pending & (1UL << SIGINT)) {
    //     current->signal_pending &= ~(1UL << SIGINT);

    //     if (current->sigactions[SIGINT].handler == (uintptr_t)SIG_IGN)
    //         return;

    //     if (current->sigactions[SIGINT].handler == (uintptr_t)SIG_DFL)
    //         do_exit(128 + SIGINT);

    //     setup_user_signal_frame(SIGINT, current->sigactions[SIGINT].handler);
    //     return;
    // }

    // if (pending & (1UL << SIGCHLD)) {
    //     current->signal_pending &= ~(1UL << SIGCHLD);

    //     if (current->sigactions[SIGCHLD].handler == (uintptr_t)SIG_IGN)
    //         return;

    //     if (current->sigactions[SIGCHLD].handler == (uintptr_t)SIG_DFL)
    //         return;

    //     setup_user_signal_frame(SIGCHLD, current->sigactions[SIGCHLD].handler);
    //     return;
    // }
}