#ifndef __ASM_PROCESS_H
#define __ASM_PROCESS_H

#include <asm/thread_info.h>
#include <asm/ptrace.h>

void show_regs(struct pt_regs *regs);
void start_thread(struct pt_regs *regs, unsigned long entry, unsigned long sp);

int copy_thread(unsigned long clone_flags, unsigned long stack_start,
	    unsigned long stk_sz, struct task_struct *p);

#endif
