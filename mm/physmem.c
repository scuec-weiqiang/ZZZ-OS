/**
 * @FilePath: /ZZZ-OS/mm/physmem.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-20 20:57:07
 * @LastEditTime: 2025-11-25 21:53:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/list.h"
#include "os/pfn.h"
#include "os/types.h"
#include <os/mm/physmem.h>
#include <os/mm/memblock.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/mm/symbols.h>

struct page *mem_map;   // base of page array
pfn_t total_pages;
pfn_t first_pfn;
pfn_t last_pfn;

struct page *pfn_to_page(pfn_t pfn) {
    if (pfn < first_pfn || pfn >= last_pfn) return NULL;
    return &mem_map[pfn - first_pfn];
}

pfn_t page_to_pfn(struct page *p) {
    return (pfn_t)( (p - mem_map) + first_pfn );
}

struct page* phys_to_page(phys_addr_t phys_addr) {
    pfn_t pfn = phys_to_pfn(phys_addr);
    return pfn_to_page(pfn);
}

phys_addr_t page_to_phys(struct page *page) {
    if (!page) return 0;
    pfn_t pfn = page_to_pfn(page);
    return pfn_to_phys(pfn);
}

static void mark_reserved_page_by_range(phys_addr_t base, phys_addr_t size) {
    pfn_t start = phys_to_pfn(base);
    pfn_t end = phys_to_pfn((base + size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));
    if (start < first_pfn) start = first_pfn;
    if (end > last_pfn) end = last_pfn;
    for (pfn_t p = start; p < end; p++) {
        struct page *pg = pfn_to_page(p);
        if (pg) pg->flags = PAGE_RESERVED;
    }
}

void physmem_init(void) {
    if (memblock.memory.total_size == 0) {
        printk("physmem_init: No memory regions available!\n");
        return;
    }

    phys_addr_t low = UINT_MAX;
    phys_addr_t high = 0;
    
    struct memblock_region *pos;
    list_for_each_entry(pos, &memblock.memory.regions, struct memblock_region, node) {
        if (pos->flags != 0) {
            continue;
        }
        if (pos->base < low) {
            low = pos->base;
        }
        if (pos->base + pos->size > high) {
            high = pos->base + pos->size;
        }
    }
    printk("Physical memory range: [%x - %x]\n", low, high);
    first_pfn = phys_to_pfn(low);
    last_pfn = phys_to_pfn(high);
    total_pages = last_pfn - first_pfn;
    printk("Total pages: %u\n", total_pages);

    size_t sz = total_pages * sizeof(struct page);
    mem_map = (struct page *)memblock_alloc(sz, PAGE_SIZE);
    for (pfn_t i = 0; i < total_pages; i++) {
        struct page *pg = &mem_map[i];
        pg->flags = 0;
        pg->order = 0xFFFF;
        pg->refcount = 0;
        INIT_LIST_HEAD(&pg->buddy_node);
    }

    list_for_each_entry(pos, &memblock.reserved.regions, struct memblock_region, node) {
        mark_reserved_page_by_range(pos->base, pos->size);
    }    
    mark_reserved_page_by_range(kernel_start, kernel_size);
    mark_reserved_page_by_range((phys_addr_t)mem_map, sz);
}