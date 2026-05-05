/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/clint.c
 * @Description:
 */
#include <asm/clint.h>
#include <asm/interrupt.h>
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/of.h>
#include <os/printk.h>
#include <os/timekeeping.h>

struct riscv64_local_irq_data {
    struct device_node *np;
    struct irq_chip *chip;
    struct irq_domain *domain;
};

static struct riscv64_local_irq_data local_irq_data;

static void riscv64_local_irq_enable(struct irq_chip *self, int hwirq)
{
    (void)self;

    switch (hwirq) {
    case 0:
        s_global_interrupt_enable();
        break;
    case CLINT_IRQ_SOFT:
        s_soft_interrupt_enable();
        break;
    case CLINT_IRQ_TIMER:
        s_timer_interrupt_enable();
        break;
    case CLINT_IRQ_EXTERN:
        s_extern_interrupt_enable();
        break;
    default:
        break;
    }
}

static void riscv64_local_irq_disable(struct irq_chip *self, int hwirq)
{
    (void)self;

    switch (hwirq) {
    case 0:
        s_global_interrupt_disable();
        break;
    case CLINT_IRQ_SOFT:
        s_soft_interrupt_disable();
        break;
    case CLINT_IRQ_TIMER:
        s_timer_interrupt_disable();
        break;
    case CLINT_IRQ_EXTERN:
        s_extern_interrupt_disable();
        break;
    default:
        break;
    }
}

static void riscv64_local_irq_set_pending(struct irq_chip *self, int hwirq)
{
    (void)self;

    switch (hwirq) {
    case CLINT_IRQ_SOFT:
        s_soft_interrupt_pending();
        break;
    case CLINT_IRQ_TIMER:
        s_timer_interrupt_pending();
        break;
    case CLINT_IRQ_EXTERN:
        s_extern_interrupt_pending();
        break;
    default:
        break;
    }
}

static void riscv64_local_irq_clear_pending(struct irq_chip *self, int hwirq)
{
    (void)self;

    switch (hwirq) {
    case CLINT_IRQ_SOFT:
        s_soft_interrupt_clear_pending();
        break;
    case CLINT_IRQ_TIMER:
        s_timer_interrupt_clear_pending();
        break;
    case CLINT_IRQ_EXTERN:
        s_extern_interrupt_clear_pending();
        break;
    default:
        break;
    }
}

static int riscv64_local_irq_get_pending(struct irq_chip *self, int hwirq)
{
    (void)self;

    switch (hwirq) {
    case CLINT_IRQ_SOFT:
        return s_soft_interrupt_get_pending();
    case CLINT_IRQ_TIMER:
        return s_timer_interrupt_get_pending();
    case CLINT_IRQ_EXTERN:
        return s_extern_interrupt_get_pending();
    default:
        return 0;
    }
}

static void riscv64_local_irq_set_priority(struct irq_chip *self, int hwirq, int priority)
{
    (void)self;
    (void)hwirq;
    (void)priority;
}

static int riscv64_local_irq_get_priority(struct irq_chip *self, int hwirq)
{
    (void)self;
    (void)hwirq;
    return 0;
}

static struct irq_ops riscv64_local_irq_ops = {
    .enable = riscv64_local_irq_enable,
    .disable = riscv64_local_irq_disable,
    .get_pending = riscv64_local_irq_get_pending,
    .set_pending = riscv64_local_irq_set_pending,
    .clear_pending = riscv64_local_irq_clear_pending,
    .set_priority = riscv64_local_irq_set_priority,
    .get_priority = riscv64_local_irq_get_priority,
};

static struct irq_chip_ops riscv64_local_irq_chip_ops = {
    .irq = &riscv64_local_irq_ops,
};

int riscv64_local_irq_init(void)
{
    int virq;

    if (local_irq_data.domain != NULL) {
        return 0;
    }

    local_irq_data.np = of_find_node_by_path("/");
    if (local_irq_data.np == NULL) {
        return -1;
    }

    local_irq_data.chip = irq_chip_register(local_irq_data.np, &riscv64_local_irq_chip_ops, NULL);
    if (local_irq_data.chip == NULL) {
        return -1;
    }

    local_irq_data.domain = irq_domain_create(local_irq_data.np,
                                              irq_domain_alloc_virq_base(RISCV64_CLINT_IRQ_COUNT),
                                              RISCV64_CLINT_IRQ_COUNT);
    if (local_irq_data.domain == NULL) {
        return -1;
    }

    virq = riscv64_local_irq_map(0);
    if (virq < 0) {
        return -1;
    }
    irq_enable(virq);

    return 0;
}

int riscv64_local_irq_map(unsigned int hwirq)
{
    int virq;

    if (local_irq_data.domain == NULL || local_irq_data.chip == NULL) {
        return -1;
    }
    if (hwirq >= RISCV64_CLINT_IRQ_COUNT) {
        return -1;
    }

    virq = irq_domain_get_virq(local_irq_data.np, hwirq);
    if (virq >= 0) {
        return virq;
    }

    if (irq_set_hwirq_and_chip(local_irq_data.domain, hwirq, local_irq_data.chip) < 0) {
        return -1;
    }

    return irq_domain_add_mapping(local_irq_data.domain, hwirq);
}

int riscv64_local_irq_dispatch(reg_t ctx, unsigned int hwirq)
{
    int virq;

    virq = irq_domain_get_virq(local_irq_data.np, hwirq);
    if (virq < 0) {
        printk("riscv64: unmapped local irq %d\n", hwirq);
        return -1;
    }

    do_irq(ctx, (void *)(uintptr_t)virq);
    return 0;
}

irqreturn_t s_soft_interrupt_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    sip_w(sip_r() & ~SIP_SSIP);
    return IRQ_HANDLED;
}

irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    timekeeping_timer_interrupt();
    return IRQ_HANDLED;
}
