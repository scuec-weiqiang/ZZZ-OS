/**
 * @FilePath: /ZZZ-OS/include/os/mm/memblock.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-14 16:02:51
 * @LastEditTime: 2025-11-21 00:30:29
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_MEMBLOCK_H
#define __KERNEL_MEMBLOCK_H

#include <os/types.h>
#include <os/list.h>

#define INIT_MEMBLOCK_REGIONS 256 

enum memblock_flags {
	MEMBLOCK_NONE		= 0x0,	/* No special request */
	MEMBLOCK_HOTPLUG	= 0x1,	/* hotpluggable region */
	MEMBLOCK_MIRROR		= 0x2,	/* mirrored region */
	MEMBLOCK_NOMAP		= 0x4,	/* don't add to kernel direct mapping */
	MEMBLOCK_DRIVER_MANAGED = 0x8,	/* always detected via a driver */
};

struct memblock_region {
    phys_addr_t base;
    size_t size;
    int flags;
    struct list_head node;
    union {
        const int idx;
        int __idx;
    };
};

struct memblock_type {
    // unsigned long cnt;	/* number of regions */
	// unsigned long max;	/* size of the allocated array */
	size_t total_size;	/* size of all regions */
    struct list_head regions;
};

struct memblock {
    struct memblock_type memory;
    struct memblock_type reserved;
};

extern struct memblock memblock;

extern void memblock_init(void);
extern int memblock_add(phys_addr_t base, size_t size);
extern int memblock_reserve(phys_addr_t base, size_t size);
extern int memblock_remove(phys_addr_t base, size_t size);
extern void memblock_mark_nomap(phys_addr_t base, size_t size);
// extern void memblock_mark_reusable(phys_addr_t base, size_t size);
extern void *memblock_alloc(size_t size, int align);
extern uint64_t memblock_phys_total(void);
extern void memblock_dump(void);

#endif