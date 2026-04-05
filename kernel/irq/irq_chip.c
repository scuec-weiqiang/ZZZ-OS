/**
 * @FilePath: /ZZZ-OS/kernel/irq/irq_chip.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-12 01:16:43
 * @LastEditTime: 2025-11-13 21:33:10
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq_chip.h>
#include <os/of_irq.h>
#include <os/string.h>
#include <os/kmalloc.h>

struct list_head irq_chip_list = LIST_HEAD_INIT(irq_chip_list);

struct irq_chip* irq_chip_register(struct device_node *np, struct irq_chip_ops *ops, void *priv) {
    if (!np || !ops) {
        return NULL;
    }
    struct irq_chip *chip = (struct irq_chip *)kmalloc(sizeof(struct irq_chip));
    chip->of_node = np;
    chip->ops = ops;
    chip->priv = priv;

    list_add(&irq_chip_list, &chip->link);
    return chip;
}

struct irq_chip* irq_chip_lookup(struct device_node *np) {
    struct irq_chip *chip;
    struct list_head *pos;

    list_for_each(pos, &irq_chip_list) {
        chip = list_entry(pos, struct irq_chip, link);
        if (chip->of_node == np) {
            return chip;
        }
    }
    return NULL;
}

extern struct of_device_id __irqchip_of_table[];

void irq_chip_init() {
    of_irq_init(__irqchip_of_table);
}