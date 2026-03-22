/**
 * @FilePath     : /ZZZ-OS/include/os/device.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:06:13
 * @LastEditTime : 2026-03-23 00:55:48
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_DEVICE_H
#define __KERNEL_DEVICE_H

#include <os/types.h>
#include <os/devicetable.h>

struct device_driver;
struct bus_type;
struct device;
struct device_node;

struct bus_type {
    const char *name;
    struct device *dev_root; // 根设备
    int (*match)(struct device *dev, struct device_driver *drv); // 匹配函数
    struct list_head drivers; // 该总线上的驱动列表
    struct list_head devices; // 该总线上的设备列表
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
    struct bus_type *bus;            // 关联的总线
    struct device_node *of_node;   // 对应 device tree 节点
    struct device *parent;          // optional
    struct device_driver *driver;          // 关联的驱动
    void *driver_data;              // optional
    struct list_head node;         // 链接到总线的设备列表
};

#endif