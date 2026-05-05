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
#include <os/of_address.h>
#include <os/of_irq.h>
#include <os/bus.h>

const struct of_device_id of_default_bus_match_table[] = {
	{ .compatible = "simple-bus", },
	{} /* Empty terminated list */
};

struct platform_device *platform_device_create(struct device_node *np, struct device *parent) {
    struct platform_device *pdev = NULL;
    struct resource temp_res;
    int i = 0, num_reg = 0, num_irq = 0;

    pdev = kmalloc(sizeof(struct platform_device));
    memset(pdev, 0, sizeof(struct platform_device));
    
    pdev->id = -1;
    pdev->dev.of_node = np;
    pdev->dev.bus = &platform_bus_type;
    pdev->dev.name = strdup(np->name);
    pdev->dev.parent = parent ? parent : &platform_bus;
    pdev->name  = pdev->dev.name;

    // printk("platform device: %s\n", pdev->name);
	while (of_address_to_resource(np, num_reg, &temp_res)) {
        // printk("  - reg: start=%xu, size=%xu\n", temp_res.start, temp_res.size);
        num_reg++;
    }

    num_irq = of_irq_count(np);
		
    if (num_reg || num_irq) {
        pdev->num_resources = num_reg + num_irq;
        pdev->resources = kmalloc(sizeof(struct resource) * pdev->num_resources);
        // printk("register platform device: %s\n", pdev->name);
        for (; i < num_reg; i++) {
            of_address_to_resource(np, i, &pdev->resources[i]);
            // printk("  - reg: start=%xu, size=%xu\n", pdev->resources[i].start, pdev->resources[i].size);
        }

        for (;i < num_reg + num_irq; i++) {
            of_irq_to_resource(np, i - num_reg, &pdev->resources[i]);
            // printk("  - irq: irq=%d\n", pdev->resources[i].irq);
        }

    }
    
    platform_device_register(pdev);
    return pdev;
}


int of_platform_bus_create(struct device_node *bus, const struct of_device_id *matches, struct device *parent) {
    struct device_node *child;
	struct platform_device *pdev;
    int rc = 0;

    if (of_get_property_by_name(bus, "compatible") == NULL) {
        return 0;
    }

    pdev = platform_device_create(bus, parent);
		// printk("   create: %s\n", bus->full_path);

	if (!pdev || !of_match_node(matches, bus))
		return 0;

	for_each_child_of_node(bus, child) {
		rc = of_platform_bus_create(child, matches,  &pdev->dev);
		if (rc) {
			break;
		}
	}

    of_node_set_flag(bus, OF_POPULATED_BUS);
    return 0;
}


void of_platform_populate(struct device_node *root, const struct of_device_id *matches, struct device *parent) {
    if (!root) {
        root = (struct device_node *)fdt_root_node;
    }
    bus_register(&platform_bus_type);
    
    struct device_node *child;
    int rc = 0;

    for_each_child_of_node(root, child) {
		rc = of_platform_bus_create(child, matches, parent);
		if (rc)
			break;
	}

    of_node_set_flag(root, OF_POPULATED_BUS);
}