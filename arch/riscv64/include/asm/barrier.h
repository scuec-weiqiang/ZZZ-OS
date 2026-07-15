#ifndef __ASM_RISCV64_BARRIER_H
#define __ASM_RISCV64_BARRIER_H

#include <os/compiler.h>

#define mb() barrier()
#define rmb() mb()
#define wmb() mb()

static inline void sfence_vma(void)
{
    asm volatile("sfence.vma zero, zero" ::: "memory");
}

#include <asm-generic/barrier.h>

#endif
