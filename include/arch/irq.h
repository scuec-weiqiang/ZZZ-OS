#ifndef __ARCH_IRQ_H__
#define __ARCH_IRQ_H__

#include <os/types.h>
#include <os/list.h>

struct irq_chip_ops {
    void (*init)(void);                   // 初始化中断控制器
    int (*ack)(void);
    void (*eoi)(unsigned int hwirq);
    void (*enable)(int hwirq);             // 使能中断
    void (*disable)(int hwirq);            // 禁用中断
    void (*pending)(int hwirq);            // 设置中断为挂起状态
    void (*set_priority)(int hwirq, int priority); // 设置中断优先级
    int  (*get_priority)(int hwirq);       // 获取中断优先级
};

struct irq_chip {
    char name[32];                       // 中断控制器名称
    struct irq_chip_ops *ops;            // 中断控制器操作函数指针
    // struct list_head *link;          // 链表节点 
};

#endif // __ARCH_IRQ_H__