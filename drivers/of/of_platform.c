/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/drivers/of/of_platform.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-11 01:44:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/fdt.h>
#include <os/list.h>
#include <os/string.h>
#include <os/of.h>
#include <os/resource.h>
#include <os/kmalloc.h>
#include <os/bswap.h>
#include <os/printk.h>
#include <os/platform_device.h>
#include <os/device.h>

const struct of_device_id of_default_bus_match_table[] = {
	{ .compatible = "simple-bus", },
	{} /* Empty terminated list */
};

struct platform_device *platform_device_create(struct device_node *np, struct device *parent) {
    if (!np) return 0;
    struct platform_device *pdev = NULL;
    uint32_t *regs = of_get_reg(np);
    // struct device_prop *interrupts = of_get_prop_by_name(np, "interrupts");
    // int irq_count = interrupts ? 1:0;

    pdev = kmalloc(sizeof(struct platform_device));
    memset(pdev, 0, sizeof(struct platform_device));
   
    pdev->dev.of_node = np;
    pdev->dev.bus = &platform_bus_type;
    pdev->dev.name = strdup(np->name);
    pdev->dev.parent = parent;

    pdev->name  = pdev->dev.name;
    
    if (regs) {
        pdev->resources = kmalloc(sizeof(struct resource));
        pdev->resources[0].type = RESOURCE_MEM;
        pdev->resources[0].start = be32_to_cpu(regs[0]); // careful with 64-bit
        pdev->resources[0].size = be32_to_cpu(regs[1]);  // if your fdt_get_reg returns pair
        pdev->num_resources = 1;
    }

    platform_device_register(pdev);
    
    return pdev;
}

// static struct device_node *queue[sizeof(struct device_node*)*512] = {0};

// void of_platform_populate1() {
//     int front=0,rear=0;
//     struct device_node *node=NULL ,*child=NULL;
//     struct platform_device *pdev=NULL;
//     struct platform_device *parent=NULL;

//     queue[rear] = (struct device_node *)fdt_root_node;
//     rear++;    

//     while(front < rear) {
//         node = queue[front];
//         front++;

//         if (node == fdt_root_node) {
//             goto next;
//         }

//         if (of_device_is_available(node) < 0) {
//             goto next;
//         }
        
//         if (of_device_is_type(node, "soc")==0 
//         || of_device_is_type(node, "simple-bus")==0 
//         || of_device_is_type(node, "memory")==0
//         || of_device_is_type(node, "cpu")==0 
//         || of_get_prop_by_name(node, "compatible") == NULL) {
//             goto next;
//         }

//         pdev = platform_device_create(node,parent);
//         printk("Found platform device: %s\n", pdev->name);
//         parent = pdev;

//         next:
//         child = node->children;
//         while(child) {
//             queue[rear] = child;
//             rear++;
//             child = child->sibling; 
//         }
//     }
// }

int of_platform_bus_create(struct device_node *bus, const struct of_device_id *matches, struct device *parent) {
    struct device_node *child;
	struct platform_device *pdev;
    int rc = 0;

    if (of_get_prop_by_name(bus, "compatible") == NULL) {
        return 0;
    }

    pdev = platform_device_create(bus, parent);

	if (!pdev || !of_match_node(matches, bus))
		return 0;

	for_each_child_of_node(bus, child) {
		printk("   create child: %s\n", child->full_path);
		rc = of_platform_bus_create(child, matches,  &pdev->dev);
		if (rc) {
			break;
		}
	}
    return 0;
}


void of_platform_populate(struct device_node *root, const struct of_device_id *matches, struct device *parent) {
    if (!root) {
        root = (struct device_node *)fdt_root_node;
    }
    
    struct device_node *child;
    int rc = 0;

    for_each_child_of_node(root, child) {
		rc = of_platform_bus_create(child, matches, parent);
		if (rc)
			break;
	}

}