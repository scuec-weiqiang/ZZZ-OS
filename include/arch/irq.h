#ifndef __ARCH_IRQ_H__
#define __ARCH_IRQ_H__

#include <os/types.h>
#include <os/list.h>

struct irq_chip {
    char name[32];                       // 中断控制器名称
    void (*init)(void);                  // 初始化控制器
    void (*enable)(int hwirq);             // 使能中断
    void (*disable)(int hwirq);            // 禁用中断
    void (*pending)(int hwirq);            // 设置中断为挂起状态
    void (*set_priority)(int hwirq, int priority); // 设置中断优先级
    int  (*get_priority)(int hwirq);       // 获取中断优先级
    struct list_head *link;          // 链表节点 
};

extern struct list_head irq_chip;

#endif // __ARCH_IRQ_H__