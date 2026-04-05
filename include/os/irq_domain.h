/**
 * @FilePath: /ZZZ-OS/include/os/irq_domain.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-01 00:38:49
 * @LastEditTime: 2025-11-14 00:28:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQ_DOMAIN_H__
#define __KERNEL_IRQ_DOMAIN_H__

#include <os/irq_chip.h>
#include <os/list.h>

struct irq_domain {
    struct device_node *np;   // 设备树节点
    // struct irq_chip *chip;   // 关联的irq_chip
    int virq_base;        // 虚拟中断号起始值
    int hw_irq_count;     // 支持的硬件中断数量
    int *hw_to_virq;      // HW IRQ→VIRQ映射表（数组）
    struct list_head link;          // 链表节点
};

extern int irq_domain_alloc_virq_base(unsigned int count);
extern struct irq_domain *irq_domain_create(struct device_node *np, unsigned int virq_base, unsigned int hw_irq_count);
extern void irq_domain_destroy(struct irq_domain *domain);
extern int irq_domain_add_mapping(struct irq_domain *domain, unsigned int hwirq);
extern struct irq_domain *irq_domain_lookup(unsigned int virq);
extern int irq_domain_get_virq(struct device_node *np, unsigned int hwirq);
extern int irq_domain_get_hwirq(struct irq_domain *domain, int virq );
extern int irq_set_hwirq_and_chip(struct irq_domain *domain, unsigned int hwirq, struct irq_chip *chip);
extern struct irq_domain* irq_find_host(struct device_node *np);
// extern struct irq_domain* irq_domain_lookup_by_hwirq( unsigned int hwirq);
#endif