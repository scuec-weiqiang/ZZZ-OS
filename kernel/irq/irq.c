/**
 * @FilePath: /ZZZ-OS/kernel/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 14:53:42
 * @LastEditTime: 2025-11-14 01:34:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <arch/irq.h>

#define IRQ_COUNT 128
struct irq_desc irq_desc[IRQ_COUNT];

void irq_init(void) {
    for (int i=0; i<IRQ_COUNT; i++) {
        irq_desc[i].name = NULL;
        irq_desc[i].virq = -1;
        irq_desc[i].handler = NULL;
        irq_desc[i].dev_id = NULL;
    }
    arch_irq_init();
}


void irq_enable(int virq) {
    struct irq_domain *domain = irq_domain_lookup(virq);
    int hwirq = irq_domain_get_hwirq(domain, virq);
    domain->chip->ops->enable(domain->chip,hwirq);
}

void irq_disable(int virq) {
    struct irq_domain *domain = irq_domain_lookup(virq);
    int hwirq = irq_domain_get_hwirq(domain, virq);
    domain->chip->ops->disable(domain->chip,hwirq);
}

void irq_acknowledge(int virq) {
    struct irq_domain *domain = irq_domain_lookup(virq);
    domain->chip->ops->ack(domain->chip);
}

void irq_set_priority(int virq, int priority) {
    struct irq_domain *domain = irq_domain_lookup(virq);
    int hwirq = irq_domain_get_hwirq(domain, virq);
    domain->chip->ops->set_priority(domain->chip, hwirq, priority);
}

int irq_get_priority(int virq) {
    struct irq_domain *domain = irq_domain_lookup(virq);
    int hwirq = irq_domain_get_hwirq(domain, virq);
    return domain->chip->ops->get_priority(domain->chip,hwirq);
}

reg_t do_irq(reg_t ctx,void *arg) {
    int virq = (int)(uintptr_t)arg;
    if (virq < 0 || virq >= IRQ_COUNT) {
        return ctx;
    }
    irq_handler_t handler = irq_desc[virq].handler;
    if (handler) {
        return handler(virq, irq_desc[virq].dev_id);
    }
    return ctx;
}

int irq_register(int virq, irq_handler_t handler, const char *name, void *dev_id) {
    if (virq < 0 || virq >= IRQ_COUNT) {
        return -1;
    }
    irq_desc[virq].handler = handler;
    irq_desc[virq].name = name;
    irq_desc[virq].dev_id = dev_id;
    irq_desc[virq].virq = virq;
    irq_desc[virq].domain = irq_domain_lookup(virq);
    irq_set_priority(virq, 1);
    return 0;
}