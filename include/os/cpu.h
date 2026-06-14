#ifndef __OS_CPU_H
#define __OS_CPU_H
#include <asm/cpu.h>

#define MAX_CPUS 8

/* SMP 辅助核 release 数据结构
 * 必须与各架构 boot.S 中的偏移量保持一致：
 *   offset 0: go (8B)
 *   offset 8: stack (8B)
 *   offset 16: entry (8B)
 */
struct secondary_data {
    unsigned long go;
    void *stack;
    void (*entry)(int cpuid, void *dtb);
};

/* CPU online bitmap — 每 bit 代表一个 CPU 是否已上线 */
extern unsigned long cpu_online_map;

static inline int cpu_online(int cpu) {
    return (int)((cpu_online_map >> (unsigned long)cpu) & 1UL);
}

static inline void set_cpu_online(int cpu) {
    cpu_online_map |= (1UL << (unsigned long)cpu);
}

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

int smp_get_cpu_count(void);

/* 各架构实现的 SMP 接口 */
void arch_secondary_init(void);
void arch_cpu_up(int cpu);
void arch_smp_init(void);

/* kernel/kernel.c 中的辅助核入口 */
void secondary_entry(int cpuid, void *dtb);

#endif