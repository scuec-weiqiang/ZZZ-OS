/**
 * @FilePath: /ZZZ-OS/drivers/irq_chip/plic/plic.c
 * @Description:
 */
#include "riscv_plic.h"
#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/irq.h>
#include <os/cpu.h>
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/of.h>
#include <os/of_address.h>
#include <os/printk.h>

struct riscv64_plic_data {
    struct device_node *np;
    struct irq_chip *chip;
    struct irq_domain *domain;
};

static struct riscv64_plic_data plic_data;
virt_addr_t plic_base = 0;

static void riscv64_plic_enable(struct irq_chip *self, int hwirq)
{
    (void)self;
    __plic_interrupt_enable(get_cpuid(), hwirq);
}

static void riscv64_plic_disable(struct irq_chip *self, int hwirq)
{
    (void)self;
    __plic_interrupt_disable(get_cpuid(), hwirq);
}

static int riscv64_plic_claim(struct irq_chip *self)
{
    (void)self;
    return __plic_claim(get_cpuid());
}

static void riscv64_plic_eoi(struct irq_chip *self, int hwirq)
{
    (void)self;
    __plic_complete(get_cpuid(), hwirq);
}

static void riscv64_plic_set_pending(struct irq_chip *self, int hwirq)
{
    (void)self;
    (void)hwirq;
}

static int riscv64_plic_get_pending(struct irq_chip *self, int hwirq)
{
    (void)self;
    return __plic_pending_get(hwirq);
}

static void riscv64_plic_clear_pending(struct irq_chip *self, int hwirq)
{
    (void)self;
    (void)hwirq;
}

static void riscv64_plic_set_priority(struct irq_chip *self, int hwirq, int priority)
{
    (void)self;
    __plic_threshold_set(get_cpuid(), 0);
    __plic_priority_set(hwirq, priority);
}

static int riscv64_plic_get_priority(struct irq_chip *self, int hwirq)
{
    (void)self;
    return __plic_priority_get(hwirq);
}

static void riscv64_plic_cpu_enable(struct irq_chip *self, int cpu)
{
    (void)self;
    __plic_threshold_set(cpu, 0);
}

static struct irq_ops riscv64_plic_irq_ops = {
    .enable = riscv64_plic_enable,
    .disable = riscv64_plic_disable,
    .get_pending = riscv64_plic_get_pending,
    .set_pending = riscv64_plic_set_pending,
    .clear_pending = riscv64_plic_clear_pending,
    .set_priority = riscv64_plic_set_priority,
    .get_priority = riscv64_plic_get_priority,
};

static struct irq_cpuif_ops riscv64_plic_cpuif_ops = {
    .claim = riscv64_plic_claim,
    .eoi = riscv64_plic_eoi,
    .cpu_enable = riscv64_plic_cpu_enable,
};

static struct irq_chip_ops riscv64_plic_chip_ops = {
    .irq = &riscv64_plic_irq_ops,
    .cpuif = &riscv64_plic_cpuif_ops,
};

static reg_t handle_plic_irq(reg_t *ctx)
{
    int hwirq;
    int virq;

    (void)ctx;

    hwirq = plic_data.chip->ops->cpuif->claim(plic_data.chip);
    if (hwirq <= 0) {
        return 0;
    }

    virq = irq_domain_get_virq(plic_data.np, hwirq);
    if (virq >= 0) {
        do_irq(0, (void *)(uintptr_t)virq);
    } else {
        printk("riscv64: invalid PLIC hwirq %d\n", hwirq);
    }

    plic_data.chip->ops->cpuif->eoi(plic_data.chip, hwirq);
    return 0;
}

static int riscv_plic_of_init(struct device_node *np, struct device_node *parent)
{
    u32 *ndev_prop;
    int irq_count;
    int virq;

    (void)parent;

    if (riscv64_local_irq_init() < 0) {
        return -1;
    }

    plic_base = (virt_addr_t)of_iomap(np, 0);
    if (plic_base == 0) {
        return -1;
    }

    plic_data.chip = irq_chip_register(np, &riscv64_plic_chip_ops, NULL);
    if (plic_data.chip == NULL) {
        return -1;
    }

    ndev_prop = of_read_u32_array(np, "riscv,ndev", 1);
    if (ndev_prop == NULL) {
        return -1;
    }
    irq_count = (int)ndev_prop[0] + 1;

    plic_data.domain = irq_domain_create(np, irq_domain_alloc_virq_base(irq_count), irq_count);
    if (plic_data.domain == NULL) {
        return -1;
    }

    plic_data.np = np;
    plic_data.chip->ops->cpuif->cpu_enable(plic_data.chip, get_cpuid());
    set_handle_irq(handle_plic_irq);

    virq = riscv64_local_irq_map(CLINT_IRQ_EXTERN);
    if (virq < 0) {
        return -1;
    }
    irq_enable(virq);

    return 0;
}

IRQCHIP_DECLARE(riscv_plic, "riscv,plic", riscv_plic_of_init);
