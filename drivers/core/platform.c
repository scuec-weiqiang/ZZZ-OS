#include <os/device.h>
#include <os/of.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/bus.h>

struct device platform_bus = {
	.name	= "platform",
};

static int platform_match(struct device *dev, const struct device_driver *drv) {
    // 根据设备树节点的 compatible 属性和驱动的 of_match_table 进行匹配
    if (!dev || !drv || !drv->of_match_table) {
        return -1;
    }
	return of_match_node(drv->of_match_table, dev->of_node) != NULL;
}

struct bus_type platform_bus_type = {
    .name = "platform",
    .match = platform_match,
};

int platform_bus_init() {
    device_register(&platform_bus);
    return bus_register(&platform_bus_type);
}



int platform_driver_register(struct platform_driver *pdrv) {
    if (!pdrv) {
        return -1;
    }
    list_add(&driver_list, &pdrv->link);

    struct platform_device *pdev;
    list_for_each_entry(pdev, &platform_device_list, struct platform_device, link) {
        if (device_matches_driver(pdev, pdrv)) {
            if (!pdev->dev->driver_data) {
                if (pdrv->probe(pdev) == 0) {
                    pdev->dev->driver_data = pdrv;
                    return 0;
                }
            }
        }
    }
    printk("platform_driver_register: no matching device for driver %s\n", pdrv->name);
    return -1;
}

int platform_driver_unregister(struct platform_driver *pdrv) {
    if (!pdrv) {
        return -1;
    }
    list_del(&pdrv->link);
    pdrv->remove(NULL);
    return 0;
}


