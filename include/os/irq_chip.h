/**
 * @FilePath: /ZZZ-OS/include/os/irq_chip.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-12 01:31:28
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQ_H__
#define __KERNEL_IRQ_H__

#include <os/types.h>
#include <os/list.h>

struct irq_chip;

struct irq_chip_ops {
    void (*init)(struct irq_chip* self);                   // 初始化中断控制器
    int  (*ack)(struct irq_chip* self);
    void (*eoi)(struct irq_chip* self, int hwirq);
    void (*enable)(struct irq_chip* self, int hwirq);             // 使能中断
    void (*disable)(struct irq_chip* self, int hwirq);            // 禁用中断
    int  (*get_pending)(struct irq_chip* self, int hwirq);            // 获取并清除中断挂起状态
    void (*set_pending)(struct irq_chip* self, int hwirq);            // 设置中断为挂起状态
    void (*clear_pending)(struct irq_chip* self, int hwirq);         // 清除中断挂起状态
    void (*set_priority)(struct irq_chip* self, int hwirq, int priority); // 设置中断优先级
    int  (*get_priority)(struct irq_chip* self, int hwirq);       // 获取中断优先级
};

struct irq_chip {
    char name[32];                       // 中断控制器名称
    int hart;
    struct irq_chip_ops *ops;            // 中断控制器操作函数指针
    struct list_head link;          // 链表节点 
    void *priv;
};

extern struct list_head irq_chip_list;
extern struct irq_chip* irq_chip_register(char *name, struct irq_chip_ops *ops, int hart, void *priv);
extern struct irq_chip* irq_chip_lookup(char *name);

#endif // __ARCH_IRQ_H__