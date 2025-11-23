/**
 * @FilePath: /ZZZ-OS/mm/physmem.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-20 20:57:07
 * @LastEditTime: 2025-11-21 16:44:30
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/mm/physmem.h>
#include <os/mm/memblock.h>

struct page *mem_map;   // base of page array
pfn_t total_pages;
pfn_t first_pfn;
pfn_t last_pfn;

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
    // mem_map = (struct page *)memblock_alloc_array(total_pages, sizeof(struct page), PAGE_SIZE);

}

struct page *pfn_to_page(pfn_t pfn) {

}

pfn_t page_to_pfn(struct page *p) {

}