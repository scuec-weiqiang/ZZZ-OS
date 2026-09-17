/**
 * @FilePath     : /ZZZ-OS/drivers/core/driver.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-25 21:34:49
 * @LastEditTime : 2026-09-14 00:53:03
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/device.h>
#include <os/bus.h>
#include <os/list.h>
#include <os/errno.h>

void driver_init() {
    device_initcalls_run();
}

int driver_register(struct device_driver *drv) {
	int ret;

    if (!drv || !drv->bus) {
        return -EINVAL;
    }
	INIT_LIST_HEAD(&drv->node);
	ret = bus_add_driver(drv);
	if (ret)
		return ret;

	driver_attach(drv);
    deferred_probe_trigger();
    return 0;
}

//只负责解除一个 device 与 driver 的绑定
static void device_release_driver(struct device *dev)
{
    int ret = 0;

    if (!dev || !dev->driver)
        return;

    /*
     * 此时不能提前清空 dev->driver，
     * 因为 platform_drv_remove 等包装函数需要通过它取得 pdrv。
     */
    if (dev->bus && dev->bus->remove)
        ret = dev->bus->remove(dev);
    else if (dev->driver->remove)
        ret = dev->driver->remove(dev);

    if (ret)
        printk("remove %s failed: %d\n", dev_name(dev), ret);

    /*
     * 即使 remove 报错，也必须结束绑定关系；
     * 注销过程不能留下指向已注销 driver 的指针。
     */
    dev->driver = NULL;
    dev->probe_state = DEVICE_UNBOUND;
}

void driver_unregister(struct device_driver *drv) {
    if (!drv || !drv->bus) {
        return;
    }
    struct device *dev;
    struct device *tmp;
    struct bus_type *bus = drv->bus;

    bus_remove_driver(drv);

    list_for_each_entry_safe(dev, tmp, &bus->devices, bus_node) {
        if (dev->driver == drv) {
            device_release_driver(dev);
        }
    }
    
   
}

// void device_release_driver(struct device *dev) {
//     if (!dev || !dev->driver) {
//         return;
//     }
//     struct device_driver *drv = dev->driver;
//     if (drv->remove) {
//         drv->remove(dev);
//     }
//     driver_unregister(drv);
//     dev->driver = NULL;
// }
