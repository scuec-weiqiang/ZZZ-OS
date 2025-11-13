/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-12 16:03:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:15:44
 * @LastEditTime: 2025-11-12 01:31:56
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
    struct irq_domain *domain = irq_domain_create(chip, RISCV64_CLINT_IRQ_BASE, RISCV64_CLINT_IRQ_COUNT);

    irq_domain_add_mapping(domain, 0); // 将全局中断号0也进行映射，方便控制全局中断，但是不注册中断函数

    int virq = irq_domain_add_mapping(domain, CLINT_IRQ_SOFT);
    irq_register(virq, s_soft_interrupt_handler, "s_soft_irq", NULL);

    virq = irq_domain_add_mapping(domain, CLINT_IRQ_TIMER);
    irq_register(virq, s_timer_interrupt_handler, "s_timer_irq", NULL);

    virq = irq_domain_add_mapping(domain, CLINT_IRQ_EXTERN);
    irq_register(virq, s_extern_interrupt_handler, "s_extern_irq", NULL);

    // irq_chip_register("riscv64_plic", &riscv64_plic_chip_ops, 0, NULL);
}