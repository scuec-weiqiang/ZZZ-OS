#ifndef __ASM_RISCV64_PROCESS_H
#define __ASM_RISCV64_PROCESS_H

#include <asm/ptrace.h>
#include <asm/thread_info.h>

struct task_struct;

void show_regs(struct pt_regs *regs);
void start_thread(struct pt_regs *regs, unsigned long entry, unsigned long sp);
int setup_kthread_context(int (*fn)(void *), void *arg, struct task_struct *p);
int setup_uthread_context(struct task_struct *p);

#endif
