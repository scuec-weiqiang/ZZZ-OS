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
#include <os/printk.h>

int device_attach(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
 
    struct device_driver *drv;
    list_for_each_entry(drv, &dev->bus->drivers, node) {
        if (dev->bus->match(dev, drv)) {
            dev->driver = drv;
            if (drv->bus->probe) {
                int ret = drv->bus->probe(dev);
                if (ret) {
                    printk("device: probe %s with %s failed: %d\n",
                           dev_name(dev), drv->name, ret);
                    dev->driver = NULL;
                }
                return ret;
            } else if (drv->probe) {
                int ret = drv->probe(dev);
                if (ret)
                    dev->driver = NULL;
                return ret;
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

    list_for_each_entry(dev, &bus->devices, bus_node) {
        if (bus->match(dev, drv)) {
            dev->driver = drv;
            if (drv->bus->probe) {
                int ret = drv->bus->probe(dev);
                if (ret) {
                    printk("driver: probe %s with %s failed: %d\n",
                           dev_name(dev), drv->name, ret);
                    dev->driver = NULL;
                    continue;
                }
            }
            return 0;
        }
    }

    return -1;
}
