/**
 * @FilePath     : /ZZZ-OS/include/os/platform_device.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:08:30
 * @LastEditTime : 2026-03-25 18:53:50
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_PLATFORM_DEVICE_H
#define __KERNEL_PLATFORM_DEVICE_H
#include <os/types.h>
#include <os/device.h>

struct platform_device {
    const char *name;              // 例如 "ns16550a"
    struct device dev;            // device core
    void *platform_data;           // optional
    struct resource *resources;
    int num_resources;
    
    struct list_head node;         // 链接到全局 platform device 列表
};

struct platform_driver {
    const char *name;
    struct device_driver drv; // driver core
    int (*probe) (struct platform_device *pdev);
    int (*remove) (struct platform_device *pdev);
    struct list_head node; // 链接到全局 platform driver 列表
};

extern struct bus_type platform_bus_type;
extern struct device platform_bus;
extern int platform_bus_init();
extern int platform_device_register(struct platform_device *pdev);
extern int platform_device_unregister(struct platform_device *pdev);

#endif