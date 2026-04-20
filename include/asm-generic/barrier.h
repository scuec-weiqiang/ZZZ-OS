#ifndef __ASM_GENERIC_BARRIER_H
#define __ASM_GENERIC_BARRIER_H


#ifndef barrier
#define barrier() __asm__ __volatile__("" ::: "memory")
#endif

#ifndef mb
#define mb() barrier()
#endif

#ifndef rmb
#define rmb() mb()
#endif

#ifndef wmb
#define wmb() mb()
#endif


#endif