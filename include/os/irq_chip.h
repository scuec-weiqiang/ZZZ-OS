/**
 * @FilePath: /ZZZ-OS/include/os/irq_chip.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-14 02:04:54
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

#include <os/types.h>
#include <os/list.h>
#include <arch/irq_chip_ops.h>

struct irq_chip {
    char name[32];                       // 中断控制器名称
    int hart;
    struct irq_chip_ops *ops;            // 中断控制器操作函数指针
    struct list_head link;          // 链表节点 
    void *priv;
};

extern struct list_head irq_chip_list;
extern struct irq_chip* irq_chip_register(char *name, struct irq_chip_ops *ops, int hart, void *priv);
extern struct irq_chip* irq_chip_lookup(char *name,int hart);

#endif // __KERNEL_IRQ_H__