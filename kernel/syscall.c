#include <asm/ptrace.h>
#include <os/sched.h>
#include <os/errno.h>
#include <os/syscall.h>
#include <os/syscall_num.h>
#include <os/timekeeping.h>
#include <os/uaccess.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1

extern long sys_open(struct pt_regs *ctx);
extern long sys_stat(struct pt_regs *ctx);
extern long sys_access(struct pt_regs *ctx);
extern long sys_mkdir(struct pt_regs *ctx);
extern long sys_rmdir(struct pt_regs *ctx);
extern long sys_unlink(struct pt_regs *ctx);
extern long sys_pipe(struct pt_regs *ctx);
extern long sys_dup2(struct pt_regs *ctx);
extern long sys_waitpid(struct pt_regs *ctx);
extern long sys_fork(struct pt_regs *ctx);

const syscall_fn_t syscall_table[SYSCALL_MAX] = {
#define X(nr, name) [nr] = sys_##name,
    SYSCALL_LIST
#undef X
};

long sys_exit_group(struct pt_regs *ctx)
{
    return sys_exit(ctx);
}

long sys_openat(struct pt_regs *ctx)
{
    struct pt_regs open_ctx = *ctx;

    open_ctx.r[0] = ctx->r[1];
    open_ctx.r[1] = ctx->r[2];
    return sys_open(&open_ctx);
}

long sys_newfstatat(struct pt_regs *ctx)
{
    struct pt_regs stat_ctx = *ctx;

    stat_ctx.r[0] = ctx->r[1];
    stat_ctx.r[1] = ctx->r[2];
    return sys_stat(&stat_ctx);
}

long sys_faccessat(struct pt_regs *ctx)
{
    struct pt_regs access_ctx = *ctx;

    access_ctx.r[0] = ctx->r[1];
    access_ctx.r[1] = ctx->r[2];
    return sys_access(&access_ctx);
}

long sys_mkdirat(struct pt_regs *ctx)
{
    struct pt_regs mkdir_ctx = *ctx;

    mkdir_ctx.r[0] = ctx->r[1];
    mkdir_ctx.r[1] = ctx->r[2];
    return sys_mkdir(&mkdir_ctx);
}

long sys_unlinkat(struct pt_regs *ctx)
{
    struct pt_regs unlink_ctx = *ctx;

    unlink_ctx.r[0] = ctx->r[1];
    if (ctx->r[2] & 0x200)
        return sys_rmdir(&unlink_ctx);

    return sys_unlink(&unlink_ctx);
}

long sys_pipe2(struct pt_regs *ctx)
{
    if (ctx->r[1] != 0)
        return -EINVAL;

    return sys_pipe(ctx);
}

long sys_dup3(struct pt_regs *ctx)
{
    if (ctx->r[2] != 0)
        return -EINVAL;

    return sys_dup2(ctx);
}

long sys_wait4(struct pt_regs *ctx)
{
    return sys_waitpid(ctx);
}

long sys_clone(struct pt_regs *ctx)
{
    unsigned long flags = ctx->r[0];
    unsigned long supported;

    supported = CSIGNAL | CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID;
    if ((flags & ~supported) != 0)
        return -EINVAL;

    return sys_fork(ctx);
}

long sys_clock_gettime(struct pt_regs *ctx)
{
    int clockid = (int)ctx->r[0];
    timespec_t ts;
    u64 now;

    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC)
        return -EINVAL;

    now = monotonic_ns();
    ts.tv_sec = now / NSEC_PER_SEC;
    ts.tv_nsec = now % NSEC_PER_SEC;

    if (copy_to_user((char *)ctx->r[1], (char *)&ts, sizeof(ts)) < 0)
        return -EFAULT;

    return 0;
}

long sys_clock_getres(struct pt_regs *ctx)
{
    int clockid = (int)ctx->r[0];
    timespec_t ts = {
        .tv_sec = 0,
        .tv_nsec = 1,
    };

    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC)
        return -EINVAL;

    if (ctx->r[1] == 0)
        return 0;

    if (copy_to_user((char *)ctx->r[1], (char *)&ts, sizeof(ts)) < 0)
        return -EFAULT;

    return 0;
}

void do_syscall(struct pt_regs *ctx) {
    long ret = -ENOSYS;
    u32 nr;

    if (ctx == NULL)
        return;

    nr = ctx->r[7];
    // dprintk("syscall: nr=%u\n", nr);
    current_thread_info()->syscall = nr;

    if (nr < SYSCALL_MAX && syscall_table[nr] != NULL)
        ret = syscall_table[nr](ctx);

    if (nr == SYSCALL_sigreturn) {

    } else {
        ctx->r[0] = (reg_t)ret;
    }
        
}
