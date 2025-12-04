#ifndef __ARCH_IRQ_CHIP_OPS_H__
#define __ARCH_IRQ_CHIP_OPS_H__

struct irq_chip;

struct irq_chip_ops {
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

#endif