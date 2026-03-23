/**
 * @FilePath: /ZZZ-OS/drivers/core/driver.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-14 00:30:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/types.h"
#include <os/driver_model.h>
#include <os/list.h>
#include <os/of.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/irq_domain.h>

static struct list_head driver_list = LIST_HEAD_INIT(driver_list);
struct list_head platform_device_list = LIST_HEAD_INIT(platform_device_list);




int platform_get_irq(struct platform_device *pdev, int index) {
    // struct device_node *np = pdev->of_node;
    // struct device_prop *prop = of_get_prop_by_name(np, "interrupts");
    // if (!prop)
    //     return -1;

    // uint32_t *val = of_read_u32_array(np, "interrupts", 1);
    // int hwirq = val[index];

    // struct device_node *intc = of_get_interrupt_parent(np);
    // prop = of_get_prop_by_name(intc, "compatible");
    // struct irq_chip *chip = irq_chip_lookup(prop->value, 0);
    // struct irq_domain *domain = (struct irq_domain *)chip->priv;
    // // int virq = domain->virq_base + hwirq;
    // return irq_domain_add_mapping(domain, hwirq);
    
}