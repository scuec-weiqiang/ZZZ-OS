/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/kernel/irq/irq_chip.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-12 01:16:43
 * @LastEditTime: 2025-11-12 16:48:12
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/irq_chip.h>

struct list_head irq_chip_list = LIST_HEAD_INIT(irq_chip_list);

struct irq_chip* irq_chip_register(char *name, struct irq_chip_ops *ops, int hart, void *priv) {
    if (!name || !ops) {
        return NULL;
    }
    struct irq_chip *chip = (struct irq_chip *)malloc(sizeof(struct irq_chip));
    strcpy(chip->name, name);
    chip->hart = hart;
    chip->ops = ops;
    chip->priv = priv;

    list_add(&irq_chip_list, &chip->link);
    return chip;
}

struct irq_chip* irq_chip_lookup(char *name) {
    struct irq_chip *chip;
    struct list_head *pos;

    list_for_each(pos, &irq_chip_list) {
        chip = list_entry(pos, struct irq_chip, link);
        if (strcmp(chip->name, name) == 0) {
            return chip;
        }
    }
    return NULL;
}