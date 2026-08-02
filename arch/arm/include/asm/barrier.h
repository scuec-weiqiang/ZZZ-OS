#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H

#ifndef barrier
#define barrier() __asm__ volatile("" ::: "memory")
#endif
#define mb() barrier()
#define rmb() mb()
#define wmb() mb()

#include <asm-generic/barrier.h>

#endif // __ASM_BARRIER_H
