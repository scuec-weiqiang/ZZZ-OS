/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/clint.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:18:22
 * @LastEditTime: 2025-10-31 20:14:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <arch/irq.h>
#include <asm/trap_handler.h>
#include <asm/interrupt.h>
#include <os/irq.h>


static void riscv64_clint_init(void) {
   trap_init(); 
}

static void riscv64_clint_enable(int hwirq) {
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

static void riscv64_clint_disable(int hwirq) {
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

static void riscv64_clint_pending(int hwirq) {
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

static void riscv64_clint_clear_pending(int hwirq) {
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

// CLINT不支持优先级设置
static void riscv64_clint_set_priority(int hwirq, int priority) {
}

// CLINT不支持优先级获取函数
static int riscv64_clint_get_priority() {
    return 0;
}

struct irq_chip_ops riscv64_clint_chip_ops = {
    .init = riscv64_clint_init,
    .enable = riscv64_clint_enable,
    .disable = riscv64_clint_disable,
    .pending = riscv64_clint_pending,
    .set_priority = riscv64_clint_set_priority, 
    .get_priority = riscv64_clint_get_priority, 
};

struct irq_chip clint = {
    .name = "riscv,clint",
    .ops = &riscv64_clint_chip_ops,
};