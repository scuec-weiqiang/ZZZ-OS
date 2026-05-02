/*
 *  arch/arm/include/asm/thread_info.h
 *
 *  Copyright (C) 2002 Russell King.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __ASM_ARM_THREAD_INFO_H
#define __ASM_ARM_THREAD_INFO_H

#include <os/types.h>
#include <os/pfn.h>
#include <os/compiler_attributes.h>

#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)
#define THREAD_START_SP		(THREAD_SIZE - 8)

struct task_struct;

typedef unsigned long mm_segment_t;

struct cpu_context_save {
	u32	r4;
	u32	r5;
	u32	r6;
	u32	r7;
	u32	r8;
	u32	r9;
	u32	sl;
	u32	fp;
	u32	sp;
	u32	pc;
	u32	extra[2];		/* Xscale 'acc' register, etc */
};

/*
 * low level task data that entry.S needs immediate access to.
 * __switch_to() assumes cpu_context follows immediately after cpu_domain.
 */
struct thread_info {
	struct cpu_context_save	cpu_context;	/* cpu context */
	struct task_struct	*task;		/* main task structure */
	int			preempt_count;/* 0 = 可抢占，>0 = 禁止抢占 */
	u32			cpu;		/* cpu */
	u32			syscall;	/* syscall number */
};

#define INIT_THREAD_INFO(tsk)						\
{									\
	.task		= &tsk,						\
}

#define init_thread_info	(init_thread_union.thread_info)
#define init_stack		(init_thread_union.stack)

/*
 * how to get the current stack pointer in C
 */
register unsigned long current_stack_pointer asm ("sp");


/*
 * how to get the thread information struct from C
 */
static inline struct thread_info *current_thread_info(void) __attribute_const__;

static inline struct thread_info *current_thread_info(void)
{
	return (struct thread_info *)
		(current_stack_pointer & ~(THREAD_SIZE - 1));
}

#define thread_saved_pc(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.pc))

#define thread_saved_sp(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.sp))

#define thread_saved_fp(tsk)	\
	((unsigned long)(task_thread_info(tsk)->cpu_context.r7))

#endif /* __ASM_ARM_THREAD_INFO_H */
