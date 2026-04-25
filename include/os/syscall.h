#ifndef SYSCALL_H
#define SYSCALL_H

#include <asm/ptrace.h>
extern void do_syscall(struct pt_regs *ctx);

#endif
