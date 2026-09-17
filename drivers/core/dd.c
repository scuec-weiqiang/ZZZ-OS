/**
 * @FilePath     : /ZZZ-OS/drivers/core/dd.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 21:53:09
 * @LastEditTime : 2026-09-14 00:36:51
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/device.h>
#include <os/bus.h>
#include <os/printk.h>
#include <os/err.h>

static LIST_HEAD(deferred_probe_list);

static void deferred_probe_add(struct device *dev)
{
    if (dev->probe_state == DEVICE_DEFERRED)
        return;

    dev->probe_state = DEVICE_DEFERRED;
    list_add_tail(&deferred_probe_list, &dev->deferred_node);
}

static int really_probe(struct device *dev, struct device_driver *drv)
{
    int ret;

    dev->driver = drv;
    dev->probe_state = DEVICE_PROBING;

    if (dev->bus->probe)
        ret = dev->bus->probe(dev);
    else if (drv->probe)
        ret = drv->probe(dev);
    else
        ret = -ENODEV;

    if (ret == 0) {
        dev->probe_state = DEVICE_BOUND;
        return 0;
    }

    dev->driver = NULL;

    if (ret == -EPROBE_DEFER) {
        deferred_probe_add(dev);
        return ret;
    }

    dev->probe_state = DEVICE_UNBOUND;
    return ret;
}

void deferred_probe_trigger(void)
{
    LIST_HEAD(retry_list);
    struct device *dev, *tmp;

    list_splice_init(&deferred_probe_list, &retry_list);

    list_for_each_entry_safe(dev, tmp, &retry_list, deferred_node) {
        list_del(&dev->deferred_node);

        if (!dev->registered)
            continue;

        dev->probe_state = DEVICE_UNBOUND;
        device_attach(dev);

        /*
         * 如果仍返回 -EPROBE_DEFER，
         * really_probe 会把它重新加入全局 deferred_probe_list。
         */
    }
}

int device_attach(struct device *dev) {
    if (!dev || !dev->bus) {
        return -1;
    }
 
    struct device_driver *drv;
    list_for_each_entry(drv, &dev->bus->drivers, node) {
        if (dev->bus->match(dev, drv)) {
           return really_probe(dev, drv);
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
        if (dev->driver) {
            continue;
        }
        if (bus->match(dev, drv)) {
           really_probe(dev, drv);
        }
    }

    return 0;
}
