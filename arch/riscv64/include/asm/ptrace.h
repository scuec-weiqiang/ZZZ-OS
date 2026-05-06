#ifndef __ASM_RISCV64_PTRACE_H
#define __ASM_RISCV64_PTRACE_H

#include <os/compiler.h>
#include <os/string.h>
#include <os/types.h>

#define SSTATUS_SIE  (1UL << 1)
#define SSTATUS_SPIE (1UL << 5)
#define SSTATUS_SPP  (1UL << 8)

struct pt_regs {
    reg_t ra;
    reg_t sp;
    reg_t gp;
    reg_t tp;
    reg_t t0;
    reg_t t1;
    reg_t t2;
    reg_t s0;
    reg_t s1;
    union {
        struct {
            reg_t a0;
            reg_t a1;
            reg_t a2;
            reg_t a3;
            reg_t a4;
            reg_t a5;
            reg_t a6;
            reg_t a7;
        };
        reg_t r[8];
    };
    reg_t s2;
    reg_t s3;
    reg_t s4;
    reg_t s5;
    reg_t s6;
    reg_t s7;
    reg_t s8;
    reg_t s9;
    reg_t s10;
    reg_t s11;
    reg_t t3;
    reg_t t4;
    reg_t t5;
    reg_t t6;
    reg_t sepc;
    reg_t sstatus;
    reg_t scause;
    reg_t stval;
};

static inline void pt_regs_clear(struct pt_regs *regs)
{
    memset(regs, 0, sizeof(*regs));
}

static inline void pt_regs_setup_user(struct pt_regs *regs,
                                      reg_t entry,
                                      reg_t user_sp,
                                      reg_t arg0)
{
    pt_regs_clear(regs);
    regs->sepc = entry;
    regs->sp = user_sp;
    regs->a0 = arg0;
    regs->sstatus = SSTATUS_SPIE;
}

static inline void pt_regs_set_retval(struct pt_regs *regs, long val)
{
    regs->a0 = (reg_t)val;
}

static inline int pt_regs_is_user(const struct pt_regs *regs)
{
    return (regs->sstatus & SSTATUS_SPP) == 0;
}

#define frame_pointer(regs) ((regs)->s0)

static inline unsigned long kernel_stack_pointer(struct pt_regs *regs)
{
    return regs->sp;
}

static inline unsigned long user_stack_pointer(struct pt_regs *regs)
{
    return regs->sp;
}

#define current_pt_regs() ({                                                \
    (struct pt_regs *)(((current_stack_pointer & ~(THREAD_SIZE - 1)) +      \
                        THREAD_SIZE) - sizeof(struct pt_regs));             \
})

extern void arch_user_enter(struct pt_regs *regs) __noreturn;

#endif
