#ifndef SYSCALL_H
#define SYSCALL_H

#include <asm/ptrace.h>
#include <os/types.h>

static inline unsigned long syscall_arg(struct pt_regs *ctx, int index)
{
    return ctx->r[index];
}

#define SYSCALL_DEFINE0(name)                                             \
    static long __do_sys_##name(void);                                    \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        (void)ctx;                                                        \
        return __do_sys_##name();                                         \
    }                                                                     \
    static long __do_sys_##name(void)

#define SYSCALL_DEFINE1(name, t1, a1)                                     \
    static long __do_sys_##name(t1 a1);                                   \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1)

#define SYSCALL_DEFINE2(name, t1, a1, t2, a2)                             \
    static long __do_sys_##name(t1 a1, t2 a2);                            \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0),                   \
                               (t2)syscall_arg(ctx, 1));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1, t2 a2)

#define SYSCALL_DEFINE3(name, t1, a1, t2, a2, t3, a3)                     \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3);                     \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0),                   \
                               (t2)syscall_arg(ctx, 1),                   \
                               (t3)syscall_arg(ctx, 2));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3)

#define SYSCALL_DEFINE4(name, t1, a1, t2, a2, t3, a3, t4, a4)         \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4);             \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0),                   \
                               (t2)syscall_arg(ctx, 1),                   \
                               (t3)syscall_arg(ctx, 2),                   \
                               (t4)syscall_arg(ctx, 3));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4)

#define SYSCALL_DEFINE5(name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5) \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5);      \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0),                   \
                               (t2)syscall_arg(ctx, 1),                   \
                               (t3)syscall_arg(ctx, 2),                   \
                               (t4)syscall_arg(ctx, 3),                   \
                               (t5)syscall_arg(ctx, 4));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5)

#define SYSCALL_DEFINE6(name, t1, a1, t2, a2, t3, a3, t4, a4, t5, a5, t6, a6) \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6);      \
    __SYSCALL__ long sys_##name(struct pt_regs *ctx)                      \
    {                                                                     \
        return __do_sys_##name((t1)syscall_arg(ctx, 0),                   \
                               (t2)syscall_arg(ctx, 1),                   \
                               (t3)syscall_arg(ctx, 2),                   \
                               (t4)syscall_arg(ctx, 3),                   \
                               (t5)syscall_arg(ctx, 4),                   \
                               (t6)syscall_arg(ctx, 5));                  \
    }                                                                     \
    static long __do_sys_##name(t1 a1, t2 a2, t3 a3, t4 a4, t5 a5, t6 a6)
extern void do_syscall(struct pt_regs *ctx);

#endif
