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
#include <os/fdt.h>
#include <os/types.h>
#include <os/list.h>

#define IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define IORESOURCE_TYPE_BITS	0x00001f00	/* Resource type */
// #define IORESOURCE_IO		0x00000100	/* PCI/ISA I/O ports */
#define IORESOURCE_MEM		0x00000200
// #define IORESOURCE_REG		0x00000300	/* Register offsets */
#define IORESOURCE_IRQ		0x00000400
// #define IORESOURCE_DMA		0x00000800
// #define IORESOURCE_BUS		0x00001000

#define IORESOURCE_IRQ_HIGHEDGE		(1<<0)
#define IORESOURCE_IRQ_LOWEDGE		(1<<1)
#define IORESOURCE_IRQ_HIGHLEVEL	(1<<2)
#define IORESOURCE_IRQ_LOWLEVEL		(1<<3)
#define IORESOURCE_IRQ_SHAREABLE	(1<<4)

struct resource {
    int flags;
    uintptr_t start;
    union {
        uintptr_t size;        // for MEM, IOPORT
        int irq;              // for IRQ
    };
    struct resource *parent, *sibling, *child;
};


#endif // OS_RESOURCE_H