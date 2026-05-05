/**
 * @FilePath     : /ZZZ-OS/include/os/platform_device.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:08:30
 * @LastEditTime : 2026-03-25 23:50:08
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_PLATFORM_DEVICE_H
#define __KERNEL_PLATFORM_DEVICE_H
#include <os/types.h>
#include <os/device.h>
#include <os/container_of.h>
#include <os/init.h>
#include <os/resource.h>
struct platform_device_id {
    char name[32];
    unsigned long driver_data;
};

struct platform_device {
    const char *name;              // 例如 "ns16550a"
    int id;                       // 设备实例编号，-1 表示自动分配
    struct device dev;            // device core
    const struct platform_device_id	*id_entry;
    void *platform_data;           // optional
    struct resource *resources;
    int num_resources;
    
    struct list_head node;         // 链接到全局 platform device 列表
};
#define to_platform_device(x) container_of((x), struct platform_device, dev)

struct platform_driver {
    const char *name;
    struct device_driver driver; // driver core
    const struct platform_device_id *id_table;

    int (*probe) (struct platform_device *pdev);
    int (*remove) (struct platform_device *pdev);

    struct list_head node; // 链接到全局 platform driver 列表
};

#define to_platform_driver(drv)	(container_of((drv), struct platform_driver, \
				 driver))
                 
extern struct bus_type platform_bus_type;
extern struct device platform_bus;
extern int platform_bus_init();

extern int platform_device_register(struct platform_device *pdev);
extern int platform_device_unregister(struct platform_device *pdev);
extern int platform_driver_register(struct platform_driver *drv);
extern int platform_driver_unregister(struct platform_driver *drv);

#define module_platform_driver(__platform_driver) \
    module_driver(__platform_driver, platform_driver_register, platform_driver_unregister)

extern struct resource *platform_get_resource(struct platform_device *pdev, unsigned int type, unsigned int index);
extern  virt_addr_t platform_ioremap_resource(struct platform_device *pdev, unsigned int index);
extern int platform_get_irq(struct platform_device *dev, unsigned int index);


static inline void platform_set_drvdata(struct platform_device *pdev, void *data) {
    pdev->platform_data = data;
}

static inline void *platform_get_drvdata(struct platform_device *pdev) {
    return pdev->platform_data;
}
#endif