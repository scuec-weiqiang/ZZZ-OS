#include <os/of.h>
#include <os/string.h>
#include <os/printk.h>
#include <os/bus.h>
#include <os/device.h>
#include <os/platform_device.h>
#include <os/resource.h>
#include <os/mm.h>
#include <os/irq_domain.h>
#include <os/errno.h>
#include <os/of_irq.h>
#include <os/pinctrl.h>

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

static int alloc_id(struct platform_device *pdev) {
    static int id = 0;
    return id++;
}

int platform_device_register(struct platform_device *pdev) {
    if (!pdev) {
        return -1;
    }
    if (pdev->id < 0) {
        pdev->id = alloc_id(pdev);
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

static int platform_drv_probe(struct device *_dev) {
	struct platform_driver *drv = to_platform_driver(_dev->driver);
	struct platform_device *dev = to_platform_device(_dev);

	int ret;

	ret = pinctrl_select_state(_dev, "default");
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	ret = drv->probe(dev);
		
	return ret;
}

static int platform_drv_remove(struct device *_dev) {
	struct platform_driver *drv = to_platform_driver(_dev->driver);
	struct platform_device *dev = to_platform_device(_dev);
	int ret;

	ret = drv->remove(dev);
	
	return ret;
}

int platform_driver_unregister(struct platform_driver *pdrv) {
    if (!pdrv) {
        return -1;
    }
    bus_remove_driver(&pdrv->driver);
    return 0;
}

int platform_driver_register(struct platform_driver *pdrv) {
    if (!pdrv) {
        return -1;
    }
    pdrv->driver.bus = &platform_bus_type;
    if (pdrv->probe) {
        pdrv->driver.probe = platform_drv_probe;
    }
    if (pdrv->remove) {
        pdrv->driver.remove = platform_drv_remove;
    }

    driver_register(&pdrv->driver);
    return 0;
}


struct resource *platform_get_resource(struct platform_device *pdev, unsigned int type, unsigned int index) {
    int i;

	for (i = 0; i < pdev->num_resources; i++) {
		struct resource *r = &pdev->resources[i];

		if (type == resource_type(r) && index-- == 0)
			return r;
	}
	return NULL;
}

virt_addr_t platform_ioremap_resource(struct platform_device *pdev, unsigned int index) {
    struct resource *res = platform_get_resource(pdev, IORESOURCE_MEM, index);
    if (!res) {
        return 0;
    }
    return (virt_addr_t)ioremap(res->start, res->size);
}

int platform_get_irq(struct platform_device *dev, unsigned int index) {
    return of_irq_get(dev->dev.of_node, index);
}
