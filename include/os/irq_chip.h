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
#include <os/of.h>

struct irq_chip;

struct irq_cpuif_ops {
    int  (*claim)(struct irq_chip *chip);
    void (*eoi)(struct irq_chip *chip, int hwirq);

    void (*cpu_enable)(struct irq_chip *chip, int cpu);
    void (*cpu_disable)(struct irq_chip *chip, int cpu);
    void (*set_prio_mask)(struct irq_chip *chip, int cpu, int prio);

};

struct irq_ipi_ops {
    void (*send_ipi)(struct irq_chip *chip, int target_cpu, int ipi_id);
    void (*broadcast_ipi)(struct irq_chip *chip, int ipi_id);
};

struct irq_ops {
    void (*enable)(struct irq_chip *chip, int hwirq);
    void (*disable)(struct irq_chip *chip, int hwirq);

    int  (*get_pending)(struct irq_chip *chip, int hwirq);
    void (*set_pending)(struct irq_chip *chip, int hwirq);
    void (*clear_pending)(struct irq_chip *chip, int hwirq);

    void (*set_priority)(struct irq_chip *chip, int hwirq, int prio);
    int  (*get_priority)(struct irq_chip *chip, int hwirq);

    void (*set_type)(struct irq_chip *chip, int hwirq, int type);
    void (*set_affinity)(struct irq_chip *chip, int hwirq, unsigned long cpu_mask);
};

struct irq_chip_ops {
    struct irq_ops *irq;
    struct irq_cpuif_ops *cpuif;
    struct irq_ipi_ops *ipi;
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
extern struct irq_chip* irq_chip_lookup(char *name,int hart);
extern void irq_chip_init();
#define IRQCHIP_DECLARE(name, compat, fn) OF_DECLARE_2(irqchip, name, compat, fn)

#endif // __KERNEL_IRQ_H__