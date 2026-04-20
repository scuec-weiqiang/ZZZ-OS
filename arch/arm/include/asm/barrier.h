#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H

#define barrier() __asm__ volatile("" ::: "memory")
#define mb() barrier()
#define rmb() mb()
#define wmb() mb()

#include <asm-generic/barrier.h>

#endif // __ASM_BARRIER_H