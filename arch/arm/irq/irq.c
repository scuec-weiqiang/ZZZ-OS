/**
 * @FilePath     : /ZZZ-OS/arch/arm/irq/irq.c
 * @Description  :
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 21:09:56
 * @LastEditTime : 2026-03-22 18:36:39
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */

#include <os/types.h>
#include <os/irq.h>
#include <os/of.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <asm/irq.h>

handle_arch_irq_t handle_arch_irq = NULL;

static struct device_node *arm_gic_node(void) {
    return of_find_node_by_compatible("arm,cortex-a7-gic");
}

void set_handle_irq(reg_t (*handle_irq)(reg_t *))
{
    if (handle_arch_irq) {
        return;
    }
    handle_arch_irq = handle_irq;
}

void arch_irq_cpu_init() {
    struct device_node *gic_np = arm_gic_node();
    struct irq_chip *chip;

    if (!gic_np) {
        return;
    }

    chip = irq_chip_lookup(gic_np);
    if (!chip || !chip->ops || !chip->ops->cpuif || !chip->ops->cpuif->cpu_enable) {
        return;
    }

    chip->ops->cpuif->cpu_enable(chip, 0);
}

void local_irq_enable(void) {
    asm volatile("cpsie i" : : : "memory");
}

void local_irq_disable(void) {
    asm volatile("cpsid i" : : : "memory");
}

int arch_irq_cpu_id(void) {
    uint32_t mpidr = 0;
    asm volatile("mrc p15, 0, %0, c0, c0, 5" : "=r"(mpidr));
    return (int)(mpidr & 0x3U);
}

int arch_local_irq_register(int hwirq, irq_handler_t handler, char *name, int cpu, void *dev_id) {
    struct device_node *gic_np = arm_gic_node();
    struct irq_chip *chip;
    struct irq_domain *domain;
    int virq;

    (void)cpu;

    if (!gic_np || !handler) {
        return -1;
    }

    chip = irq_chip_lookup(gic_np);
    domain = irq_find_host(gic_np);
    if (!chip || !domain) {
        return -1;
    }

    virq = irq_domain_get_virq(gic_np, hwirq);
    if (virq < 0) {
        virq = irq_domain_add_mapping(domain, hwirq);
        if (virq < 0) {
            return -1;
        }
    }

    if (irq_set_hwirq_and_chip(domain, hwirq, chip) < 0) {
        return -1;
    }

    if (irq_request(virq, handler, name, dev_id) < 0) {
        return -1;
    }

    return 0;
}

int arch_extern_irq_register(int hwirq, irq_handler_t handler, char *name, int cpu, void *dev_id){
    return arch_local_irq_register(hwirq, handler, name, cpu, dev_id);
}
