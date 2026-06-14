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
#include <asm/riscv.h>
#include <asm/barrier.h>

/* boot.S 中通过 secondary_data 符号引用 */
struct secondary_data secondary_data[MAX_CPUS];

void arch_secondary_init(void)
{
    /* 写入内核页表基址 (与 CPU0 共享 init_mm.pgdir) */
    pgtable_t *pgdir = init_mm.pgdir;
    if (pgdir) {
        /* 构造 satp 值: MODE=Sv39(8), ASID=0, PPN=pgdir>>12 */
        reg_t satp_val = (8UL << 60) | (((reg_t)(unsigned long)pgdir >> 12) & 0xFFFFFFFFFFFUL);
        satp_w(satp_val);
        sfence_vma();
    }

    /* 设置异常向量到 kernel_trap_entry */
    extern void kernel_trap_entry(void);
    stvec_w((reg_t)(unsigned long)kernel_trap_entry);

    /* 使能 S 模式中断: timer | software | external */
    sie_w(SIE_STIE | SIE_SSIE | SIE_SEIE);

    /* 开启 S 模式全局中断 (SSTATUS_SIE) */
    sstatus_w(sstatus_r() | (1 << 1));
}

void arch_cpu_up(int cpu)
{
    if (cpu <= 0 || cpu >= MAX_CPUS)
        return;

    /* stack 指针由 sched_init() -> create_idle_task() 填充 */
    secondary_data[cpu].entry = secondary_entry;

    /* 内存屏障确保写入对目标 CPU 可见 */
    __sync_synchronize();

    /* 设置 go 标志 */
    secondary_data[cpu].go = 1;

    /* 通过 CLINT MSIP 发送 IPI 唤醒目标 hart */
    RELEASE_CORE(cpu);
}

void arch_smp_init(void)
{
    int total_cpus = smp_get_cpu_count();

    printk("SMP: %d CPUs detected, bringing up secondaries...\n", total_cpus);

    for (int cpu = 1; cpu < total_cpus && cpu < MAX_CPUS; cpu++) {
        arch_cpu_up(cpu);
    }
}
