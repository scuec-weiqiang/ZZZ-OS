/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/irq_ctrl.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:15:44
 * @LastEditTime: 2025-10-31 15:10:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <asm/irq.h>
#include <asm/trap_handler.h>
#include <asm/interrupt.h>
#include <asm/clint.h>
#include <os/list.h>

extern struct irq_chip clint;

void arch_irq_init() {
}