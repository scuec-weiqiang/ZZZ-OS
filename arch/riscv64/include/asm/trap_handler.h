/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/trap_handler.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-01 16:47:45
 * @LastEditTime: 2025-11-13 21:13:22
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef TRAP_H
#define TRAP_H

#include <os/irqreturn.h>
#include <os/types.h>

struct pt_regs;

extern void trap_init(void);
extern irqreturn_t s_soft_interrupt_handler(int virq, void *dev_id);
extern irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id);
extern reg_t trap_handler(reg_t ctx);
extern struct pt_regs *trap_prepare_user_return(struct pt_regs *regs);
#endif
