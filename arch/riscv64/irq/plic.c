/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/plic.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:18:22
 * @LastEditTime: 2025-11-12 01:35:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq_chip.h>
#include <asm/trap_handler.h>
#include <asm/riscv_plic.h>
#include <asm/riscv.h>

static void riscv64_plic_init(struct irq_chip* self) {
}

static void riscv64_plic_enable(struct irq_chip* self, int hwirq) {
    __plic_interrupt_enable(self->hart, hwirq);
}

static void riscv64_plic_disable(struct irq_chip* self, int hwirq) {
    __plic_interrupt_disable(self->hart, hwirq);
}

static int riscv64_plic_ack(struct irq_chip *self) {
   return  __plic_claim(self->hart);
}

static void riscv64_plic_eio(struct irq_chip *self, int hwirq) {
    __plic_complete(self->hart, hwirq);
}

static void riscv64_plic_set_pending(struct irq_chip* self, int hwirq) {
    // 不支持
}

static int riscv64_plic_get_pending(struct irq_chip *self, int hwirq) {
    return __plic_pending_get(hwirq);
}

static void riscv64_plic_clear_pending(struct irq_chip* self, int hwirq) {
    // 不支持
}
  
static void riscv64_plic_set_priority(struct irq_chip* self, int hwirq, int priority) {
    __plic_priority_set(hwirq, priority);
}

static int riscv64_plic_get_priority(struct irq_chip* self, int hwirq) {
    return __plic_priority_get(hwirq);
}

struct irq_chip_ops riscv64_plic_chip_ops = {
    .init = riscv64_plic_init,
    .ack = riscv64_plic_ack,
    .eoi = riscv64_plic_eio,
    .enable = riscv64_plic_enable,
    .disable = riscv64_plic_disable,
    .set_pending = riscv64_plic_set_pending,
    .clear_pending = riscv64_plic_clear_pending,
    .get_pending = riscv64_plic_get_pending,
    .set_priority = riscv64_plic_set_priority, 
    .get_priority = riscv64_plic_get_priority, 
};
