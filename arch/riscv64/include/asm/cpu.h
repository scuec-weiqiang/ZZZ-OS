#ifndef __ASM_CPU_H
#define __ASM_CPU_H

#include <asm/riscv.h>

#define HAVE_ARCH_CPU

static inline int arch_get_cpuid(void)
{
    return (int)tp_r();
}

static inline int arch_get_cpu_identification(void)
{
    return (int)tp_r();
}

static inline void arch_cpu_relax(void)
{
    asm volatile("nop" ::: "memory");
}

static inline void arch_cpu_idle(void)
{
    asm volatile("wfi" ::: "memory");
}

#endif /* __ASM_CPU_H */
