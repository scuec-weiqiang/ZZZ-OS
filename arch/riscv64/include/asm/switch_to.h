#ifndef __ASM_RISCV64_SWITCH_TO_H
#define __ASM_RISCV64_SWITCH_TO_H

#include <os/types.h>

struct thread_info;
struct task_struct;

extern struct task_struct *__switch_to(struct task_struct *prev,
                                       struct thread_info *prev_thread,
                                       struct thread_info *next_thread);

#define switch_to(prev, next, last)                                          \
do {                                                                         \
    last = __switch_to(prev, task_thread_info(prev), task_thread_info(next));\
} while (0)

#endif
