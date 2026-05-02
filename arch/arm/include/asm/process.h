#ifndef __ASM_PROCESS_H
#define __ASM_PROCESS_H

#include <asm/thread_info.h>
#include <asm/ptrace.h>

void show_regs(struct pt_regs *regs);
void debug_dump_ret_from_fork(struct pt_regs *regs);

void start_thread(struct pt_regs *regs, unsigned long entry, unsigned long sp);
int setup_kthread_context(int (*fn)(void *), void *arg, struct task_struct *p);
int setup_uthread_context(struct task_struct *p);

#endif
