/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-12 01:42:45
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


void arch_irq_init() {
    trap_init();
    struct irq_chip *chip = irq_chip_register("riscv64_clint", &riscv64_clint_chip_ops, 0, NULL);
    struct irq_domain *domain = irq_domain_create(chip, RISCV64_CLINT_IRQ_BASE, RISCV64_CLINT_IRQ_COUNT);

    // irq_chip_register("riscv64_plic", &riscv64_plic_chip_ops, 0, NULL);
}