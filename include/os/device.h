/**
 * @FilePath     : /ZZZ-OS/include/os/device.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:06:13
 * @LastEditTime : 2026-03-25 22:54:50
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_DEVICE_H
#define __KERNEL_DEVICE_H

#include <os/types.h>
#include <os/devicetable.h>
#include <os/init.h>

struct device_driver;
struct bus_type;
struct device;
struct device_node;

struct bus_type {
    const char *name;
    struct device *dev_root; // 根设备
    int (*match)(struct device *dev, const struct device_driver *drv); // 匹配函数
    struct list_head drivers; // 该总线上的驱动列表
    struct list_head devices; // 该总线上的设备列表

    struct list_head node;
};

struct device_driver {
    const char *name;
    struct bus_type *bus; // 关联的总线类型
    struct of_device_id *of_match_table; // 设备树匹配表
    int (*probe) (struct device *dev);
	int (*remove) (struct device *dev);
    struct list_head node; // 链接到总线的驱动列表
};

struct device {
    const char *name;
    // int state;              
    struct bus_type *bus;            // 关联的总线
    struct device_node *of_node;   // 对应 device tree 节点
    struct device *parent;          // optional
    struct device_driver *driver;          // 关联的驱动
    void *driver_data;              // optional
    struct list_head node;         // 链接到总线的设备列表
};

static inline void *dev_get_drvdata(const struct device *dev) {
	return dev->driver_data;
}

static inline void dev_set_drvdata(struct device *dev, void *data) {
	dev->driver_data = data;
}

extern int device_register(struct device *dev);
extern int device_unregister(struct device *dev);
extern int device_add(struct device *dev);
extern int device_attach(struct device *dev);

extern int driver_register(struct device_driver *drv);
extern void driver_unregister(struct device_driver *drv);
extern int driver_attach(struct device_driver *drv);

#define module_driver(__driver, __register, __unregister) \
    static int  __driver##_init(void) \
    { \
        return __register(&(__driver)); \
    } \
    static void  __driver##_exit(void) \
    { \
        __unregister(&(__driver)); \
    } \
    module_init(__driver##_init,".initcall"); \
    module_exit(__driver##_exit,".exitcall");

extern void driver_init();

#endif