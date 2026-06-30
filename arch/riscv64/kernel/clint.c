#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/irq.h>
#include <asm/sbi.h>
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/of.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/timekeeping.h>
#include <os/cpu.h>

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

static void riscv64_local_irq_send_ipi(struct irq_chip *self, int target_cpu, int ipi_id)
{
    (void)self;

    if (ipi_id < 0 || ipi_id >= IPI_MAX || target_cpu < 0) {
        return;
    }

    sbi_ipi_send(1UL << (unsigned long)target_cpu, 0);
}

static void riscv64_local_irq_broadcast_ipi(struct irq_chip *self, int ipi_id)
{
    (void)self;

    if (ipi_id < 0 || ipi_id >= IPI_MAX) {
        return;
    }

    sbi_ipi_send((unsigned long)-1, 0);
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

static struct irq_ipi_ops riscv64_local_irq_ipi_ops = {
    .send_ipi = riscv64_local_irq_send_ipi,
    .broadcast_ipi = riscv64_local_irq_broadcast_ipi,
};

static struct irq_chip_ops riscv64_local_irq_chip_ops = {
    .irq = &riscv64_local_irq_ops,
    .ipi = &riscv64_local_irq_ipi_ops,
};

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
    if (current) {
        current->need_resched = 1;
    }
    return IRQ_HANDLED;
}

irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    timekeeping_timer_interrupt();
    return IRQ_HANDLED;
}


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

    virq = riscv64_local_irq_map(CLINT_IRQ_SOFT);
    irq_request(virq, s_soft_interrupt_handler, "riscv64_ipi", NULL);

    return 0;
}
