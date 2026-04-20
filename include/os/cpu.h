#ifndef __OS_CPU_H
#define __OS_CPU_H
#include <asm/cpu.h>

static inline int get_cpuid(void) {
    return arch_get_cpuid();
}

static inline int get_cpu_identification(void) {
    return arch_get_cpu_identification();
}

static inline void cpu_relax(void) {
    arch_cpu_relax();
}

static inline void cpu_idle(void) {
    arch_cpu_idle();
}


#endif