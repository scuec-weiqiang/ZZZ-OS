/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/trap_handler.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-01 16:47:45
 * @LastEditTime: 2025-11-12 16:36:49
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef TRAP_H
#define TRAP_H

#include <os/types.h>
#include <os/irqreturn.h>

extern void trap_init();
extern  s_soft_interrupt_handler(int virq, void *dev_id);
// extern reg_t m_soft_interrupt_handler(reg_t epc);
extern irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id);
// extern reg_t m_timer_interrupt_handler(reg_t epc);
extern irqreturn_t s_extern_interrupt_handler(int virq, void *dev_id);
// extern reg_t m_extern_interrupt_handler(reg_t epc);
extern irqreturn_t trap_handler(reg_t ctx);
#endif