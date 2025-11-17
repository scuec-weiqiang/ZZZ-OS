/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-14 02:19:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq_chip.h>
#include <asm/trap_handler.h>
#include <asm/interrupt.h>
#include <asm/clint.h>
#include <asm/plic.h>
#include <os/list.h>
#include <os/printk.h>
#include <os/malloc.h>
#include <os/irq_domain.h>
#include <os/irq.h>

void arch_irq_init() {
    trap_init();

    struct irq_chip *chip = irq_chip_register("riscv64_clint", &riscv64_clint_chip_ops, 0, NULL);
    int virq_base = irq_domain_alloc_virq_base(RISCV64_CLINT_IRQ_COUNT);
    struct irq_domain *domain = irq_domain_create(chip, virq_base, RISCV64_CLINT_IRQ_COUNT);

    irq_domain_add_mapping(domain, 0); // 将全局中断号0也进行映射，方便控制全局中断，但是不注册中断函数

    int virq = irq_domain_add_mapping(domain, CLINT_IRQ_SOFT);
    irq_register(virq, s_soft_interrupt_handler, "soft_irq", NULL);

    virq = irq_domain_add_mapping(domain, CLINT_IRQ_TIMER);
    irq_register(virq, s_timer_interrupt_handler, "timer_irq", NULL);
}

int arch_local_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id) {
    struct irq_chip *chip = irq_chip_lookup("riscv64_clint", hart);
    if (!chip) {
        return -1;
    }
    struct irq_domain *domain = (struct irq_domain *)chip->priv;
    if (!domain) {
        return -1;
    }
    int virq = irq_domain_add_mapping(domain, hwirq);
    if (virq < 0) {
        return virq;
    }
    irq_register(virq, handler, name, dev_id);
    return 0;
}

int arch_extern_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id) {
    struct irq_chip *chip = irq_chip_lookup("riscv64_plic", hart);
    if (!chip) {
        return -1;
    }
    struct irq_domain *domain = (struct irq_domain *)chip->priv;
    if (!domain) {
        return -1;
    }
    int virq = irq_domain_add_mapping(domain, hwirq);
    if (virq < 0) {
        return virq;
    }
    irq_register(virq, handler, name, dev_id);
    return 0;
}

// int arch_