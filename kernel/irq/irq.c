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
#include <os/init.h>

#include <asm/irq.h>

#define IRQ_COUNT 128
struct irq_data __irq_data[IRQ_COUNT];

static int irq_current_cpu_id(void) {
    return arch_irq_cpu_id();
}

void irq_init(void) {
    for (int i=0; i<IRQ_COUNT; i++) {
        int cpu;
        __irq_data[i].name = NULL;
        __irq_data[i].virq = -1;
        __irq_data[i].handler = NULL;
        __irq_data[i].dev_id = NULL;
        __irq_data[i].chip = NULL;
        __irq_data[i].domain = NULL;
        __irq_data[i].is_percpu = 0;
        for (cpu = 0; cpu < IRQ_PERCPU_MAX_CPUS; cpu++) {
            __irq_data[i].percpu_handler[cpu] = NULL;
            __irq_data[i].percpu_dev_id[cpu] = NULL;
        }
    }
    irq_chip_init();
    // arch_irq_init();
}

struct irq_data *irq_data_get(int virq) {
    if (virq < 0 || virq >= IRQ_COUNT) {
        return NULL;
    }
    return &__irq_data[virq];
}

void irq_enable(int virq) {
    int hwirq = irq_domain_get_hwirq(__irq_data[virq].domain, virq);
    __irq_data[virq].chip->ops->irq->enable(__irq_data[virq].chip,hwirq);
}

void irq_disable(int virq) {
    int hwirq = irq_domain_get_hwirq(__irq_data[virq].domain, virq);
    __irq_data[virq].chip->ops->irq->disable(__irq_data[virq].chip,hwirq);
}

void irq_acknowledge(int virq) {
    __irq_data[virq].chip->ops->cpuif->claim(__irq_data[virq].chip);
}

void irq_set_priority(int virq, int priority) {
    int hwirq = irq_domain_get_hwirq(__irq_data[virq].domain, virq);
    __irq_data[virq].chip->ops->irq->set_priority(__irq_data[virq].chip, hwirq, priority);
}

int irq_get_priority(int virq) {
    int hwirq = irq_domain_get_hwirq(__irq_data[virq].domain, virq);
    return __irq_data[virq].chip->ops->irq->get_priority(__irq_data[virq].chip,hwirq);
}

reg_t do_irq(reg_t ctx,void *arg) {
    int virq = (int)arg;
    int cpu_id;
    irq_handler_t handler;
    void *dev_id;

    if (virq < 0 || virq >= IRQ_COUNT) {
        return ctx;
    }

    cpu_id = irq_current_cpu_id();
    if (__irq_data[virq].is_percpu && cpu_id >= 0 && cpu_id < IRQ_PERCPU_MAX_CPUS) {
        handler = __irq_data[virq].percpu_handler[cpu_id];
        dev_id = __irq_data[virq].percpu_dev_id[cpu_id];
        if (handler) {
            return handler(virq, dev_id);
        }
    }

    handler = __irq_data[virq].handler;
    if (handler) {
        return handler(virq, __irq_data[virq].dev_id);
    }
    return ctx;
}

int irq_request(int virq, irq_handler_t handler, const char *name, void *dev_id) {
    if (virq < 0 || virq >= IRQ_COUNT) {
        return -1;
    }
    __irq_data[virq].is_percpu = 0;
    __irq_data[virq].handler = handler;
    __irq_data[virq].name = name;
    __irq_data[virq].dev_id = dev_id;
    __irq_data[virq].virq = virq;
    __irq_data[virq].domain = irq_domain_lookup(virq);
    irq_set_priority(virq, 1);
    return 0;
}

int irq_percpu_request(int virq, int cpu, irq_handler_t handler, const char *name, void *dev_id) {
    if (virq < 0 || virq >= IRQ_COUNT) {
        return -1;
    }
    if (cpu < 0 || cpu >= IRQ_PERCPU_MAX_CPUS) {
        return -1;
    }
    if (!handler) {
        return -1;
    }

    __irq_data[virq].is_percpu = 1;
    __irq_data[virq].name = name;
    __irq_data[virq].virq = virq;
    __irq_data[virq].domain = irq_domain_lookup(virq);
    __irq_data[virq].percpu_handler[cpu] = handler;
    __irq_data[virq].percpu_dev_id[cpu] = dev_id;
    irq_set_priority(virq, 1);

    return 0;
}
