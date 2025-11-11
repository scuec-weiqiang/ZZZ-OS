/**
 * @FilePath: /ZZZ-OS/include/os/driver_model.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-11 21:14:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef OS_DRIVER_MODEL_H
#define OS_DRIVER_MODEL_H

#include <os/list.h>
#include <drivers/of/fdt.h>
#include <os/types.h>
#include <os/list.h>

enum resource_type {
    RESOURCE_IRQ,
    RESOURCE_MEM,
    RESOURCE_DMA,
    RESOURCE_IOPORT,
};

struct resource {
    enum resource_type type;
    uintptr_t start;
    union {
        uintptr_t size;        // for MEM, IOPORT
        int irq;              // for IRQ
        int channel;          // for DMA
    };
    struct resource *parent, *sibling, *child;
};

struct device {
    const char *name;
    void *driver_data;             // driver-specific pointer
};

struct platform_device {
    const char *name;              // 例如 "ns16550a"
    struct device_node *of_node;   // 对应 device tree 节点
    // struct platform_device *parent;  // optional
    void *platform_data;           // optional
    struct resource *resources;
    int num_resources;
    struct device *dev;            // device core
    struct list_head link;         // 链接到全局 platform device 列表
};

extern struct list_head platform_device_list;


struct of_device_id {
    const char *compatible;
    const void *data;
};

struct platform_driver {
    const char *name;
    const struct of_device_id *of_match_table; // null-terminated
    int (*probe)(struct platform_device *pdev);
    void (*remove)(struct platform_device *pdev);
    // struct platform_driver *next;
    struct list_head link;         
};

#endif // OS_DRIVER_MODEL_H