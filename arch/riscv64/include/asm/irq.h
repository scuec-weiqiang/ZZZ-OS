/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/irq.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-27 23:02:59
 * @LastEditTime: 2025-10-28 19:40:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include <os/types.h>

struct {
    void (*handler)(unsigned int irq, void *dev_id);  // 中断处理函数
    void *dev_id;                                     // 设备ID
    const char *name;                                 // 中断名称
} irq_desc;

#endif