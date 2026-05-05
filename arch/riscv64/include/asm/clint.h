/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/clint.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-14 02:18:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __ASM_CLINT_H__
#define __ASM_CLINT_H__

#include <os/types.h>

#define RISCV64_CLINT_IRQ_COUNT 16

extern int riscv64_local_irq_init(void);
extern int riscv64_local_irq_map(unsigned int hwirq);
extern int riscv64_local_irq_dispatch(reg_t ctx, unsigned int hwirq);

#endif // __ASM_CLINT_H__
