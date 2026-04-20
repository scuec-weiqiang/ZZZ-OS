#ifndef __ASM_CPU_H
#define __ASM_CPU_H

#define HAVE_ARCH_CPU

static inline int arch_get_cpuid(void) {
    int mpidr;
    __asm__ volatile (
        "mrc p15, 0, %0, c0, c0, 5"
        : "=r"(mpidr)  // 输出：midr保存结果
        :             // 无输入
        : "memory"    // 内存屏障，防止编译器优化
    );
    return mpidr & 0xFF;
}

static inline int arch_get_cpu_identification() {
    int midr;
    __asm__ volatile (
        "mrc p15, 0, %0, c0, c0, 0"
        : "=r"(midr)  // 输出：midr保存结果
        :             // 无输入
        : "memory"    // 内存屏障，防止编译器优化
    );
    return midr;
}

static inline void arch_cpu_relax(void) {
    __asm__ volatile("yield" ::: "memory");
}

static inline void arch_cpu_idle(void) {
    __asm__ volatile("wfi" ::: "memory");
}

#endif /* __ASM_CPU_H */