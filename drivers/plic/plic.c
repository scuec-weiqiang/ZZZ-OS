/**
 * @FilePath: /ZZZ-OS/drivers/plic/plic.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-31 01:18:22
 * @LastEditTime: 2025-11-14 02:10:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/driver_model.h"
#include "riscv_plic.h"
#include <os/irq_chip.h>
#include <asm/riscv.h>
#include <drivers/core/driver.h>
#include <os/irq_domain.h>
#include <os/irqreturn.h>
#include <os/mm.h>
#include <asm/irq.h>
#include <os/irq.h>
#include <os/of.h>

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
    __plic_threshold_set(0, 0);
    __plic_priority_set(hwirq, priority);
}

static int riscv64_plic_get_priority(struct irq_chip* self, int hwirq) {
    return __plic_priority_get(hwirq);
}

struct irq_chip_ops riscv64_plic_chip_ops = {
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

static irqreturn_t extern_interrupt_handler(int virq, void *dev_id) {
    // int hart_id = tp_r();

    struct irq_chip *chip = irq_chip_lookup("riscv,plic", tp_r());
    if (chip) {
        int extern_irq = chip->ops->ack(chip);
        int virq = irq_domain_get_virq(chip, extern_irq);
        if (virq >= 0) {
            do_irq(virq, (void *)(uintptr_t)virq);
        } else {
            printk("Invalid virq!\n");
        }
        if (virq) {
            chip->ops->eoi(chip, extern_irq);
        }
        return IRQ_HANDLED;
    } else {
        printk("IRQ chip not found!\n");
    }
    
    return IRQ_NONE;
}

static int riscv_plic_probe(struct platform_device *pdev) {
    struct device_node *node = of_find_node_by_compatible("riscv,plic");
    if (!node) {
        return -1;
    }
    uint32_t *reg = of_read_u32_array(node, "riscv,ndev", 1);
    int irq_count = (int)reg[0];

    reg = of_read_u32_array(node, "reg", 2);
    ioremap(reg[0], reg[1]);

    reg = of_read_u32_array(node, "interrupts-extended", 2);
    int hwirq = (int)reg[1]; 
    
    int virq_base = irq_domain_alloc_virq_base(irq_count);
    struct irq_chip *chip = irq_chip_register("riscv,plic", &riscv64_plic_chip_ops, 0, NULL);
    irq_domain_create(chip, virq_base, irq_count);

    arch_local_irq_register(hwirq, extern_interrupt_handler, "riscv_plic_extern", 0, NULL);

    return 0;
}

static void riscv_plic_remove(struct platform_device *pdev) {
}

static struct of_device_id riscv_plic_match[] = {
    {.compatible = "riscv,plic"},
    {/* sentinel */}
};

static struct platform_driver riscv_plic_driver = {
    .name = "riscv_plic",
    .probe = riscv_plic_probe,
    .remove = riscv_plic_remove,
    .of_match_table = riscv_plic_match
};

module_platform_irq(riscv_plic_driver);

