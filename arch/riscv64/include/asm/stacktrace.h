#ifndef __ASM_RISCV64_STACKTRACE_H
#define __ASM_RISCV64_STACKTRACE_H

#include <asm/ptrace.h>

struct stackframe {
    unsigned long fp;
    unsigned long sp;
    unsigned long ra;
    unsigned long pc;
};

static __always_inline void riscv64_get_current_stackframe(struct pt_regs *regs,
                                                           struct stackframe *frame)
{
    frame->fp = frame_pointer(regs);
    frame->sp = regs->sp;
    frame->ra = regs->ra;
    frame->pc = regs->sepc;
}

extern int unwind_frame(struct stackframe *frame);
extern void walk_stackframe(struct stackframe *frame,
                            int (*fn)(struct stackframe *, void *), void *data);

#endif
