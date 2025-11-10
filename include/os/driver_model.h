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
    struct device_node *of_node;   // 对应 device tree 节点
    const char *name;              // 例如 "ns16550a"
    void *platform_data;           // optional
    struct resource *resources;
    int num_resources;
    struct device *dev;            // device core
    // struct platform_device *next;
    struct list_head link;         // 链接到全局 platform device 列表
};

struct list_head platform_device_list;


struct of_device_id {
    const char *compatible;
    const void *data;
};

struct platform_driver {
    const char *name;
    const struct of_device_id *of_match_table; // null-terminated
    int (*probe)(struct platform_device *pdev);
    int (*remove)(struct platform_device *pdev);
    // struct platform_driver *next;
    struct list_head link;         
};

#endif // OS_DRIVER_MODEL_H