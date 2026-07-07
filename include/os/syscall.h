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

extern void do_syscall(struct pt_regs *ctx);

#endif
