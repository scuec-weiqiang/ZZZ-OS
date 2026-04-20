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
#include <os/cpu.h>
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

static irqreturn_t arm_arch_ipi_handler(int virq, void *dev_id) {
    (void)virq;
    (void)dev_id;
    printk("Received IPI on CPU %d\n", get_cpuid());
    return IRQ_HANDLED;
}

void arch_irq_init() {
    struct device_node *gic_node = arm_gic_node();
    struct irq_domain *domain = irq_find_host(gic_node);
    int virq = irq_domain_add_mapping(domain, IPI_RESCHED);
    irq_request(virq, arm_arch_ipi_handler, "arm_arch_ipi", NULL);
    irq_enable(virq);
}

void local_irq_enable(void) {
    asm volatile("cpsie i" : : : "memory");
}

void local_irq_disable(void) {
    asm volatile("cpsid i" : : : "memory");
}


unsigned long arch_local_irq_save(void) {
    unsigned long flags;
    asm volatile(
        "mrs %0, cpsr\n"
        "cpsid i\n"
        : "=r"(flags)
        :
        : "memory", "cc");
    return flags;
}

void arch_local_irq_restore(unsigned long flags)
{
    asm volatile(
        "msr cpsr_c, %0"
        :
        : "r"(flags)
        : "memory", "cc");
}




