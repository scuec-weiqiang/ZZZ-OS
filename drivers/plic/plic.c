/**
 * @FilePath: /ZZZ-OS/arch/riscv64/irq/plic.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:18:22
 * @LastEditTime: 2025-11-12 01:35:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/driver_model.h"
#include "riscv_plic.h"
#include <os/irq_chip.h>
#include <asm/trap_handler.h>
#include <asm/riscv.h>
#include <drivers/core/driver.h>
#include <os/irq_domain.h>

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

static int riscv_plic_probe(struct platform_device *pdev) {
    int irq_count = 0;
    struct irq_chip *chip = irq_chip_register("riscv64_plic", &riscv64_plic_chip_ops, 0, NULL);
    struct irq_domain *domain = irq_domain_create(chip, 16, irq_count);

    irq_domain_add_mapping(domain, 0); // 将全局中断号0也进行映射，方便控制全局中断，但是不注册中断函数

    // int virq = irq_domain_add_mapping(domain, CLINT_IRQ_SOFT);
    // irq_register(virq, s_soft_interrupt_handler, "s_soft_irq", NULL);
    return 0;
}

static void riscv_plic_remove(struct platform_device *pdev) {

}

static struct of_device_id riscv_plic_match[] = {
    {.compatible = "riscv,plic"},
    {""}
};

static struct platform_driver riscv_plic_driver = {
    .name = "riscv_plic",
    .probe = riscv_plic_probe,
    .remove = riscv_plic_remove,
    .of_match_table = riscv_plic_match
};

module_platform_driver(riscv_plic_driver);

