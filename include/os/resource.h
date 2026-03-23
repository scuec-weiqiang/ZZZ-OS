/**
 * @FilePath: /ZZZ-OS/include/os/driver_model.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-11 21:14:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef OS_RESOURCE_H
#define OS_RESOURCE_H

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

#endif // OS_RESOURCE_H