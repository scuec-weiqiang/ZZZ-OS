/**
 * @FilePath     : /ZZZ-OS/arch/arm/include/asm/irq.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 18:36:39
 * @LastEditTime : 2026-03-22 23:21:46
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef __ASM_IRQ_H
#define __ASM_IRQ_H

#include <asm-generic/irq.h>

enum ipi_id{
    IPI_RESCHED = 0,
    IPI_PREEMPT,
    IPI_MAX
};

#endif // __ASM_IRQ_H
