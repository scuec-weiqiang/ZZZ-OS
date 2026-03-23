#include <os/of.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/bus.h>
#include <os/platform_device.h>

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
    .devices = LIST_HEAD_INIT(platform_bus_type.devices),
    .drivers = LIST_HEAD_INIT(platform_bus_type.drivers),
};

int platform_bus_init() {
    device_register(&platform_bus);
    return bus_register(&platform_bus_type);
}



int platform_device_register(struct platform_device *pdev) {
    if (!pdev) {
        return -1;
    }
    device_register(&pdev->dev);
    return 0;
}

int platform_device_unregister(struct platform_device *pdev) {
    if (!pdev) {
        return -1;
    }
    device_unregister(&pdev->dev);
    return 0;
}


