/**
 * @FilePath: /ZZZ-OS/include/os/irq_domain.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-01 00:38:49
 * @LastEditTime: 2025-11-01 01:47:06
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQ_DOMAIN_H__
#define __KERNEL_IRQ_DOMAIN_H__

#include <arch/irq.h>
#include <os/list.h>

struct irq_domain {
    struct irq_chip *chip;         // 关联的中断控制器
    unsigned int virq_base;        // 虚拟中断号起始值
    unsigned int hw_irq_count;     // 支持的硬件中断数量
    unsigned int *hw_to_virq;      // HW IRQ→VIRQ映射表（数组）
    struct list_head link;          // 链表节点
};

extern struct irq_domain *irq_domain_create(struct irq_chip *chip, unsigned int virq_base, unsigned int hw_irq_count);
extern void irq_domain_destroy(struct irq_domain *domain);
extern struct irq_domain *irq_domain_lookup(unsigned int virq);
extern int irq_domain_get_virq(struct irq_chip *chip, unsigned int hwirq);

#endif