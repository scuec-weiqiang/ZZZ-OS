/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq.c
 * @Description:
 */
#include <asm/clint.h>
#include <asm/irq.h>
#include <asm/riscv.h>
#include <asm/trap_handler.h>

handle_arch_irq_t handle_arch_irq = NULL;

void set_handle_irq(reg_t (*handle_irq)(reg_t *))
{
    if (handle_arch_irq != NULL) {
        return;
    }

    handle_arch_irq = handle_irq;
}

void arch_irq_init(void)
{
    trap_init();
    riscv64_local_irq_init();
}

void local_irq_enable(void)
{
    sstatus_w(sstatus_r() | 0x2UL);
}

void local_irq_disable(void)
{
    sstatus_w(sstatus_r() & ~0x2UL);
}

unsigned long arch_local_irq_save(void)
{
    unsigned long flags;

    flags = sstatus_r();
    local_irq_disable();
    return flags;
}

void arch_local_irq_restore(unsigned long flags)
{
    sstatus_w(flags);
}
