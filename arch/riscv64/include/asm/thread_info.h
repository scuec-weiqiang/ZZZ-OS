#ifndef __ASM_RISCV64_THREAD_INFO_H
#define __ASM_RISCV64_THREAD_INFO_H

#include <asm/context.h>
#include <os/compiler_attributes.h>
#include <os/pfn.h>
#include <os/types.h>

#define THREAD_SIZE_ORDER 1
#define THREAD_SIZE       (PAGE_SIZE << THREAD_SIZE_ORDER)
#define THREAD_START_SP   THREAD_SIZE

struct task_struct;

struct thread_info {
    struct context cpu_context;
    struct task_struct *task;
    int preempt_count;
    u32 cpu;
    u32 syscall;
};

#define INIT_THREAD_INFO(tsk) \
{ \
    .task = &tsk, \
}

#define init_thread_info (init_thread_union.thread_info)
#define init_stack       (init_thread_union.stack)

register unsigned long current_stack_pointer asm("sp");

static inline struct thread_info *current_thread_info(void) __attribute_const__;

static inline struct thread_info *current_thread_info(void)
{
    return (struct thread_info *)(current_stack_pointer & ~(THREAD_SIZE - 1));
}

#define thread_saved_pc(tsk) \
    ((unsigned long)(task_thread_info(tsk)->cpu_context.ra))

#define thread_saved_sp(tsk) \
    ((unsigned long)(task_thread_info(tsk)->cpu_context.sp))

#define thread_saved_fp(tsk) \
    ((unsigned long)(task_thread_info(tsk)->cpu_context.s0))

#endif /* __ASM_RISCV64_THREAD_INFO_H */
