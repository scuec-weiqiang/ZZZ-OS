/**
 * @FilePath: /ZZZ-OS/arch/arm/kernel/smp.c
 * @Description: ARM SMP bring-up (i.MX6ULL)
 */

#include <os/types.h>
#include <os/cpu.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/mm.h>
#include <asm/irq.h>

struct secondary_data secondary_data[MAX_CPUS];

void arch_secondary_init(void)
{
    int cpu = get_cpuid();

    /* 写入内核页表基址 (共享 init_mm.pgdir) */
    pgtable_t *pgdir = init_mm.pgdir;
    if (pgdir) {
        /* TTBR0 = pgdir 物理地址 */
        unsigned long ttbr0 = (unsigned long)pgdir;
        asm volatile("mcr p15, 0, %0, c2, c0, 0" :: "r"(ttbr0) : "memory");

        /* 设置域访问控制 */
        unsigned long dacr = 0x55555555;
        asm volatile("mcr p15, 0, %0, c3, c0, 0" :: "r"(dacr) : "memory");

        /* 刷新 TLB */
        asm volatile(
            "mov r0, #0\n\t"
            "mcr p15, 0, r0, c8, c7, 0\n\t"
            "dsb\n\t"
            "isb\n\t"
            ::: "r0", "memory"
        );
    }

    /* 启用 MMU */
    unsigned long sctlr;
    asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
    sctlr |= 1;                     /* SCTLR.M */
    sctlr |= (1 << 2);              /* SCTLR.C (data cache) */
    sctlr |= (1 << 12);             /* SCTLR.I (instruction cache) */
    asm volatile("mcr p15, 0, %0, c1, c0, 0" :: "r"(sctlr) : "memory");
    asm volatile("isb" ::: "memory");
}

void arch_cpu_up(int cpu)
{
    if (cpu <= 0 || cpu >= MAX_CPUS)
        return;

    /*
     * i.MX6ULL 上辅助核由 SRC (System Reset Controller) 控制。
     * 需要写 SRC_GPR 和 SRC_SCR 寄存器来释放。
     * TODO: 添加 SRC 驱动后实现真正的 bring-up。
     */
    printk("SMP: CPU%d bring-up not yet supported on ARM (needs SRC driver)\n", cpu);
}

void arch_smp_init(void)
{
    int total_cpus = smp_get_cpu_count();

    printk("SMP: %d CPUs detected\n", total_cpus);

    for (int cpu = 1; cpu < total_cpus && cpu < MAX_CPUS; cpu++) {
        arch_cpu_up(cpu);
    }
}
