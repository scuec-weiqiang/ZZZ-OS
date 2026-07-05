/**
 * @FilePath: /ZZZ-OS/arch/riscv64/kernel/smp.c
 * @Description: RISC-V SMP bring-up
 */

#include <os/types.h>
#include <os/cpu.h>
#include <os/printk.h>
#include <os/irq.h>
#include <os/sched.h>
#include <os/mm.h>
#include <os/kva.h>
#include <mm/pgtbl.h>
#include <asm/trap_handler.h>
#include <asm/riscv.h>
#include <asm/barrier.h>
#include <asm/sbi.h>

/* boot.S 中通过 secondary_data 符号引用 */
struct secondary_data secondary_data[MAX_CPUS];
extern void secondary_start(int cpuid);
extern void secondary_entry(int cpuid);

void arch_secondary_init(void)
{
    /* 写入内核页表基址 (与 CPU0 共享 init_mm.pgdir) */
    pgtable_t *pgdir = init_mm.pgdir;
    if (pgdir) {
        pgtbl_switch_to(pgdir);
    }

    /* 设置异常向量到 kernel_trap_entry */
    trap_init();
    
    sie_w(SIE_SSIE | SIE_STIE);
}

void arch_cpu_up(int cpu)
{
    long err;

    if (cpu < 0 || cpu >= MAX_CPUS)
        return;

    if (secondary_data[cpu].stack == NULL) {
        printk("SMP: CPU%d has no idle stack\n", cpu);
        return;
    }
    secondary_data[cpu].entry = secondary_entry;


    /* 内存屏障确保写入对目标 CPU 可见 */
    __sync_synchronize();

    secondary_data[cpu].go = 1;

    err = sbi_hart_start((unsigned long)cpu,
                         (unsigned long)KERNEL_PA((unsigned long)secondary_start),
                         0);
    if (err != SBI_SUCCESS) {
        printk("SMP: failed to start CPU%d via SBI HSM, err=%d\n", cpu, err);
    }
}

void arch_smp_init(int boot_cpu) {
    int total_cpus = smp_get_cpu_count();

    printk("SMP: %d CPUs detected, bringing up secondaries...\n", total_cpus);

    if (sbi_probe_extension(SBI_EXT_HSM) <= 0) {
        printk("SMP: SBI HSM extension not available\n");
        return;
    }

    for (int cpu = 0; cpu < total_cpus && cpu < MAX_CPUS; cpu++) {
        if (cpu == boot_cpu) {
            continue; // boot CPU 已经启动
        }
        arch_cpu_up(cpu);
    }
}
