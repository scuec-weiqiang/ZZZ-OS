#include <os/device.h>
#include <os/list.h>
#include <os/of.h>

int device_add(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
    list_add(&dev->bus->devices, &dev->node);
    dev->driver = NULL; // 先不关联驱动，等 driver_register 时再匹配
    return 0;
}

int device_attach(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
 
    struct device_driver *drv;
    list_for_each_entry(drv, &dev->bus->drivers, struct device_driver, node) {
        if (dev->bus->match(dev, drv)) {
            dev->driver = drv;
            if (drv->probe) {
                return drv->probe(dev);
            }
        }
    }

    return 0;
}

int device_register(struct device *dev) {
    if (!dev) {
        return -1;
    }
    device_add(dev);
    return device_attach(dev);
}

int device_unregister(struct device *dev) {
    if (!dev) {
        return -1;
    }
    list_del(&dev->node);
    return 0;
}