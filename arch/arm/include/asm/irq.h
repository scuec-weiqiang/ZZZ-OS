/**
 * @FilePath     : /ZZZ-OS/arch/arm/include/asm/irq.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 18:36:39
 * @LastEditTime : 2026-03-22 23:21:46
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include "os/irq.h"
#include <os/irqreturn.h>
#include <os/types.h>

extern void arch_irq_cpu_init();
extern int arch_local_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id);
extern int arch_extern_irq_register(int hwirq, irq_handler_t handler, char *name, int hart, void *dev_id);

#endif // _ASM_IRQ_H