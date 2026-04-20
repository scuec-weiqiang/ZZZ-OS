#ifndef __ASM_CONTEXT_H
#define __ASM_CONTEXT_H
#include <os/types.h>
#include <os/string.h>

struct thread_info;
struct task_struct;
// extern  struct task_struct* __switch_to(struct task_struct*, struct thread_info* , struct thread_info* );

// #define switch_to(prev,next) __switch_to(prev,task_thread_info(prev), task_thread_info(next));

extern struct task_struct *__switch_to(struct task_struct *, struct thread_info *, struct thread_info *);

#define switch_to(prev,next,last)					\
do {									\
	last = __switch_to(prev,task_thread_info(prev), task_thread_info(next));	\
} while (0)

#endif
