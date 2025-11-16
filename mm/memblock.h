/**
 * @FilePath: /ZZZ-OS/mm/memblock.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-14 16:02:51
 * @LastEditTime: 2025-11-14 16:10:12
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_MEMBLOCK_H
#define __KERNEL_MEMBLOCK_H

#include <os/types.h>

struct mem_region {
    phys_addr_t start;
    size_t size;
    int type; // 0: usable, 1: reserved
};

struct memblock {
    struct mem_region memory[32];
    int memory_count;

    struct mem_region reserved[32];
    int reserved_count;
};

extern struct memblock memblock;

extern void memblock_init(void);
extern void memblock_add(phys_addr_t base, size_t size);
extern void memblock_reserve(phys_addr_t base, size_t size);
extern uint64_t memblock_phys_total(void);
extern void memblock_dump(void);

#endif