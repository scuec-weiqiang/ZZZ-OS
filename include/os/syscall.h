#ifndef SYSCALL_H
#define SYSCALL_H

#include <os/proc.h>
extern void do_syscall(struct trap_frame *ctx);

#endif