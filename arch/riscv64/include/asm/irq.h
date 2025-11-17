/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/irq.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-12 16:41:47
 * @LastEditTime: 2025-11-13 22:55:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include "os/irq.h"
#include <os/irqreturn.h>
#include <os/types.h>

extern void arch_irq_init();
extern int arch_local_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id);
extern int arch_extern_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id);

#endif // _ASM_IRQ_H