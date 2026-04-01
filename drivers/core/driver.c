/**
 * @FilePath     : /ZZZ-OS/drivers/core/driver.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 21:34:49
 * @LastEditTime : 2026-03-25 23:02:04
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/device.h>
#include <os/bus.h>
#include <os/list.h>
#include <mm/symbols.h>

void driver_init() {
    do_initcalls(initcall_start, initcall_end)
}

int driver_register(struct device_driver *drv) {
    if (!drv || !drv->bus) {
        return -1;
    }
    bus_add_driver(drv);
    driver_attach(drv);
    return 0;
}


void driver_unregister(struct device_driver *drv) {
    if (!drv || !drv->bus) {
        return;
    }
    bus_remove_driver(drv);
}

void device_release_driver(struct device *dev) {
    if (!dev || !dev->driver) {
        return;
    }
    struct device_driver *drv = dev->driver;
    if (drv->remove) {
        drv->remove(dev);
    }
    driver_unregister(drv);
    dev->driver = NULL;
}
