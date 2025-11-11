/**
 * @FilePath: /ZZZ-OS/kernel/irq/of_irq.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-12 01:03:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/of.h>
#include <os/driver_model.h>
#include <os/irq_domain.h>

void of_irq_init(void) {
    if(of_find_node_by_compatible("riscv,clint")) {
        
    }
}

int platform_get_irq(struct platform_device *pdev, int index)
{
    struct device_node *np = pdev->of_node;
    struct device_prop *prop = fdt_get_prop_by_name(np, "interrupts");
    if (!prop)
        return -1;

    uint32_t *val = of_read_u32_array(np, "interrupts", 1);
    
    return be32_to_cpu(val[index]); // 简化
}