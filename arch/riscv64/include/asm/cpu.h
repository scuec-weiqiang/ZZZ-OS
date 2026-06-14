#ifndef __ASM_CPU_H
#define __ASM_CPU_H

#include <asm/riscv.h>

#define HAVE_ARCH_CPU

#define CLINT_BASE          0x02000000
#define CLINT_MTIME                 (CLINT_BASE + (0xbff8))
#define CLINT_MTIMECMP_BASE         (CLINT_BASE + (0x4000))
#define CLINT_MSIP(cpuid)          (CLINT_BASE + 4*(cpuid))
#define RELEASE_CORE(cpuid)        (*(u32*)CLINT_MSIP(cpuid)=1)

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
