/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/drivers/of/of_platform.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-11 01:44:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <drivers/of/fdt.h>
#include <os/list.h>
#include <os/string.h>
#include <os/of.h>
#include <os/driver_model.h>
#include <os/kmalloc.h>
#include <os/bswap.h>
#include <os/printk.h>

struct platform_device *platform_device_create_from_node(struct device_node *np) {
    if (!np) return 0;
    struct platform_device *pdev = NULL;
    struct device_prop *compatible = of_get_prop_by_name(np, "compatible");
    uint32_t *regs = of_get_reg(np);
    // struct device_prop *interrupts = of_get_prop_by_name(np, "interrupts");
    // int irq_count = interrupts ? 1:0;

    pdev = kmalloc(sizeof(struct platform_device));
    pdev->dev = kmalloc(sizeof(struct device));
    pdev->of_node = np;
    pdev->name  = strdup(compatible->value);
    
    if (regs) {
        pdev->resources = kmalloc(sizeof(struct resource));
        pdev->resources[0].type = RESOURCE_MEM;
        pdev->resources[0].start = be32_to_cpu(regs[0]); // careful with 64-bit
        pdev->resources[0].size = be32_to_cpu(regs[1]);  // if your fdt_get_reg returns pair
        pdev->num_resources = 1;
    }
    list_add(&platform_device_list, &pdev->link);
    return pdev;
}
static struct device_node *queue[sizeof(struct device_node*)*512] = {0};
void of_platform_populate() {
    int front=0,rear=0;
    struct device_node *node=NULL ,*child=NULL;
    struct platform_device *pdev=NULL;
    // struct device_prop *prop=NULL;

    queue[rear] = (struct device_node *)fdt_root_node;
    rear++;    


    while(front < rear) {
        node = queue[front];
        front++;

        if (node == fdt_root_node) {
            goto next;
        }

        if (of_device_is_available(node) < 0) {
            goto next;
        }
        
        if (of_device_is_type(node, "soc")==0 
        || of_device_is_type(node, "simple-bus")==0 
        || of_device_is_type(node, "memory")==0
        || of_device_is_type(node, "cpu")==0 
        || of_get_prop_by_name(node, "compatible") == NULL) {
            goto next;
        }

        pdev = platform_device_create_from_node(node);
        printk("Found platform device: %s\n", pdev->name);

        next:
        child = node->children;
        while(child) {
            queue[rear] = child;
            rear++;
            child = child->sibling; 
        }
    }
}