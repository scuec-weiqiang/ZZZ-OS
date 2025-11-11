/**
 * @FilePath: /ZZZ-OS/include/os/irq.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 14:39:47
 * @LastEditTime: 2025-11-12 00:55:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef KERNEL_IRQ_H
#define KERNEL_IRQ_H

#include <os/types.h>
#include <os/irqreturn.h>

struct irq_chip;
struct irq_domain;

typedef irqreturn_t (*irq_handler_t)(int virq, void *dev_id);

struct irq_desc {
    const char *name;                   // 调试名称
    int virq;                     // 逻辑中断号
    // int hwirq;                          // 硬件中断号
    // struct irq_chip *chip;        // 指向控制器私有数据
    // struct irq_domain *domain;    // 所属中断域
    irq_handler_t *handler;        // 中断处理函数
    void *dev_id;                  // 设备标识符         
};

extern void irq_init(void);
extern int irq_register(int virq, irq_handler_t handler, const char *name, void *dev_id);
extern void irq_enable(int virq);
extern void irq_disable(int virq);
extern reg_t irq_ack(int virq);
extern void irq_set_priority(int virq, int priority);
extern int irq_get_priority(int virq);


#endif // KERNEL_IRQ_H