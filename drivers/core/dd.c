/**
 * @FilePath     : /ZZZ-OS/drivers/core/dd.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 21:53:09
 * @LastEditTime : 2026-03-25 22:58:42
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/device.h>
#include <os/bus.h>

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

int driver_attach(struct device_driver *drv) {
    struct bus_type *bus = drv->bus;
    struct device *dev;

    if (!bus) {
        return -1;
    }

    list_for_each_entry(dev, &bus->devices, struct device, node) {
        if (bus->match(dev, drv)) {
            dev->driver = drv;
            if (drv->probe) {
                // printk("Attaching driver %s to device %s\n", drv->name, dev->name);
                return drv->probe(dev);
            }
            return 0;
        }
    }

    return -1;
}