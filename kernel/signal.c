#include <os/types.h>
#include <os/sched.h>
#include <os/signal.h>
#include <os/err.h>
#include <os/uaccess.h>
#include <os/magic.h>

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

int setup_signal_frame(int sig, signalfn_t handler, struct pt_regs *ctx) {
    struct signal_frame frame;
    uintptr_t tramp = current->signal_trampoline;

    if (tramp == 0)
        return -EINVAL;

    frame.magic = SIGF;
    frame.signo = sig;
    frame.saved_tf = *ctx;
    frame.old_blocked = current->signal_blocked;

    uintptr_t usp = ctx->sp;
    usp -= sizeof(frame);
    usp &= ~7;   // 8-byte align

    if (copy_to_user((void*)usp, (void*)&frame, sizeof(frame)) < 0)
        return -EFAULT;

    /*
     * 修改即将恢复到用户态的现场
     */
    ctx->r[0] = sig;                         // handler 第一个参数
    ctx->pc = (reg_t)handler;                     // 返回用户态后执行 handler
    ctx->lr = tramp;                      // handler return 后跳这里
    ctx->sp = usp;

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

    if (pending & (1UL << SIGINT)) {
        current->signal_pending &= ~(1UL << SIGINT);

        if (current->sigactions[SIGINT].handler == (signalfn_t)SIG_IGN)
            return;

        if (current->sigactions[SIGINT].handler == (signalfn_t)SIG_DFL)
            do_exit(128 + SIGINT);

        setup_signal_frame(SIGINT, current->sigactions[SIGINT].handler, regs);
        return;
    }

    if (pending & (1UL << SIGCHLD)) {
        current->signal_pending &= ~(1UL << SIGCHLD);

        if (current->sigactions[SIGCHLD].handler == (signalfn_t)SIG_IGN)
            return;

        if (current->sigactions[SIGCHLD].handler == (signalfn_t)SIG_DFL)
            return;

        setup_signal_frame(SIGCHLD, current->sigactions[SIGCHLD].handler, regs);
        return;
    }
}

long sys_sigreturn(struct pt_regs *ctx) {
    struct signal_frame frame;
    uintptr_t usp = ctx->sp;

    if (copy_from_user((void*)&frame, (void*)usp, sizeof(frame)) < 0)
        do_exit(SIGSEGV);

    if (frame.magic != 0x53494746)
        do_exit(SIGSEGV);

    current->signal_blocked = frame.old_blocked;

    *ctx = frame.saved_tf;

    return 0;
}

long sys_sigaction(struct pt_regs *ctx) {
    int sig = ctx->r[0];
    struct k_sigaction *act = (struct k_sigaction *)ctx->r[1];
    struct k_sigaction *oldact = (struct k_sigaction *)ctx->r[2];

    if (sig < 0 || sig >= NSIG)
        return -EINVAL;

    if (oldact) {
        if (copy_to_user((void*)oldact, (void*)&current->sigactions[sig], sizeof(struct k_sigaction)) < 0)
            return -EFAULT;
    }

    if (act) {
        struct k_sigaction new_act;
        if (copy_from_user((void*)&new_act, (void*)act, sizeof(struct k_sigaction)) < 0)
            return -EFAULT;

        current->sigactions[sig] = new_act;
    }

    return 0;
}

void send_signal(struct task_struct *t, int sig) {
    if (sig < 0 || sig >= NSIG)
        return;

    t->signal_pending |= 1UL << sig;
    // wake_up_process(t);
}