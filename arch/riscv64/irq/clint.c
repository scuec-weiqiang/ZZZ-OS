/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/clint.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:18:22
 * @LastEditTime: 2025-10-31 20:14:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq_chip.h>
#include <asm/trap_handler.h>
#include <asm/interrupt.h>

static void riscv64_clint_init(struct irq_chip* self) {
   trap_init(); 
}

static int riscv64_clint_ack(struct irq_chip *self) {
   return  0;
}

static void riscv64_clint_eio(struct irq_chip *self, int hwirq) {
    
}

static void riscv64_clint_enable(struct irq_chip* self, int hwirq) {
    switch (hwirq) {
    case CLINT_IRQ_SOFT: // 软件中断
        s_soft_interrupt_enable();
        break;
    case CLINT_IRQ_TIMER: // 定时器中断
        s_timer_interrupt_enable();
        break;
    case CLINT_IRQ_EXTERN: // 外部中断
        s_extern_interrupt_enable();
        break;
    default:
        break;
    }
}

static void riscv64_clint_disable(struct irq_chip* self, int hwirq) {
    switch (hwirq) {
    case CLINT_IRQ_SOFT: // 软件中断
        s_soft_interrupt_disable();
        break;
    case CLINT_IRQ_TIMER: // 定时器中断
        s_timer_interrupt_disable();
        break;
    case CLINT_IRQ_EXTERN: // 外部中断
        s_extern_interrupt_disable();
        break;
    default:
        break;
    }
}

static void riscv64_clint_set_pending(struct irq_chip* self, int hwirq) {
    switch (hwirq) {
    case CLINT_IRQ_SOFT: // 软件中断
        s_soft_interrupt_pending();
        break;
    case CLINT_IRQ_TIMER: // 定时器中断
        s_timer_interrupt_pending();
        break;
    case CLINT_IRQ_EXTERN: // 外部中断
        s_extern_interrupt_pending();
        break;
    default:
        break;
    }
}

static void riscv64_clint_clear_pending(struct irq_chip* self, int hwirq) {
    switch (hwirq) {
    case CLINT_IRQ_SOFT: // 软件中断
        s_soft_interrupt_clear_pending();
        break;
    case CLINT_IRQ_TIMER: // 定时器中断
        s_timer_interrupt_clear_pending();
        break;
    case CLINT_IRQ_EXTERN: // 外部中断
        s_extern_interrupt_clear_pending();
        break;
    default:
        break;
    }
}

static int riscv64_clint_get_pending(struct irq_chip *self, int hwirq) {
    int ret = 0;
    switch (hwirq) {
    case CLINT_IRQ_SOFT: // 软件中断
        ret = s_soft_interrupt_get_pending();
        break;
    case CLINT_IRQ_TIMER: // 定时器中断
        ret = s_timer_interrupt_get_pending();
        break;
    case CLINT_IRQ_EXTERN: // 外部中断
        ret = s_extern_interrupt_get_pending();
        break;
    default:
        break;
    }
    return ret;
}

// CLINT不支持优先级设置
static void riscv64_clint_set_priority(struct irq_chip* self, int hwirq, int priority) {
}

// CLINT不支持优先级获取函数
static int riscv64_clint_get_priority(struct irq_chip* self, int hwirq) {
    return 0;
}

struct irq_chip_ops riscv64_clint_chip_ops = {
    .init = riscv64_clint_init,
    .ack = riscv64_clint_ack,
    .eoi = riscv64_clint_eio,   
    .enable = riscv64_clint_enable,
    .disable = riscv64_clint_disable,
    .set_pending = riscv64_clint_set_pending,
    .clear_pending = riscv64_clint_clear_pending,
    .get_pending = riscv64_clint_get_pending,
    .set_priority = riscv64_clint_set_priority, 
    .get_priority = riscv64_clint_get_priority, 
};
