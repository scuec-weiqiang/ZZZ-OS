/**
 * @FilePath: /ZZZ-OS/mm/memblock.c
 * @Description: 
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-14 16:00:57
 * @LastEditTime: 2025-11-17 00:20:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "os/types.h"
#include <os/mm/memblock.h>
#include <os/printk.h>
#include <os/list.h>
#include <os/pfn.h>

// #define MARK_NOMAP(x) ((x).flags |= 0x1)
// #define MARK_REUSABLE(x) ((x).flags &= ~0x1)
#define MEMBLOCK_NONE     0
#define MEMBLOCK_NOMAP    (1 << 0)
#define MEMBLOCK_REUSABLE    (1 << 1)

#define IS_NOMAP(flags) ((flags) & MEMBLOCK_NOMAP)

static struct memblock_region memblock_regions_pool[INIT_MEMBLOCK_REGIONS];

static int free_idx_stack[INIT_MEMBLOCK_REGIONS];
static int free_idx_top;

static struct memblock_region* memblock_region_alloc() {
    if (free_idx_top < 0) {
        return NULL;
    }
    struct memblock_region* region = &memblock_regions_pool[free_idx_stack[free_idx_top]];
    free_idx_top--;
    return region;
}

static void memblock_region_free(struct memblock_region* region) {
    if (free_idx_top >= INIT_MEMBLOCK_REGIONS - 1 || !region) {
        return;
    }

    region->base = 0;
    region->size = 0;
    region->flags = 0;
    INIT_LIST_HEAD(&region->node);

    free_idx_top++;
    free_idx_stack[free_idx_top] = region->__idx;
}

#define alloc_region()      memblock_region_alloc()
#define free_region(region) __PROTECT(memblock_region_free((region));(region) = NULL; )

struct memblock memblock;

static void memblock_sort_one(struct memblock_type *type, struct memblock_region *region) {
    struct memblock_region *pos = NULL, *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (region != pos && region->base < pos->base) {
            list_del(&region->node);
            list_add_before(&pos->node ,&region->node);
            return;
        }
    }
}

static void memblock_sort(struct memblock_type *type) {
    struct memblock_region *region = NULL, *tmp = NULL;
    list_for_each_entry_safe(region, tmp, &type->regions, struct memblock_region, node) {
        memblock_sort_one(type, region);
    }
}

static unsigned long memblock_addrs_overlap(phys_addr_t base1, size_t size1, phys_addr_t base2, size_t size2) {
	return ((base1 < (base2 + size2)) && (base2 < (base1 + size1)));
}

static int memblock_overlaps_region(struct memblock_type *type, phys_addr_t base, size_t size) {
    struct memblock_region* pos = NULL, *n = NULL;
    list_for_each_entry_safe(pos, n, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            return pos->__idx;
        }
    }
    return -1;
}

static void memblock_merge_regions(struct memblock_type *type, struct memblock_region *region) {
    phys_addr_t base = region->base;
    size_t size = region->size;

    struct memblock_region *pos = NULL, *tmp = NULL;

    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (pos != region) {
            phys_addr_t rgnbase = pos->base;
            size_t rgnsize = pos->size;

            if (memblock_addrs_overlap(base, size, rgnbase, rgnsize)) {
                phys_addr_t new_base = (base < rgnbase) ? base : rgnbase;
                phys_addr_t new_end = ((base + size) > (rgnbase + rgnsize)) ? (base + size) : (rgnbase + rgnsize);
                base = new_base;
                size = new_end - new_base;

                // 移除合并的区域
                list_del(&pos->node);
                free_region(pos);

                // 更新合并后的区域
                region->base = base;
                region->size = size;
            }
        }
    }
}


void memblock_init(void) {
    for (int i = 0; i < INIT_MEMBLOCK_REGIONS; i++) {
        memblock_regions_pool[i].base = 0;
        memblock_regions_pool[i].size = 0;
        memblock_regions_pool[i].flags = 0;
        memblock_regions_pool[i].__idx = i;
        INIT_LIST_HEAD(&memblock_regions_pool[i].node);

        free_idx_stack[i] = i;
    } 
    free_idx_top = INIT_MEMBLOCK_REGIONS - 1;
    
    memblock.memory.cnt = 0;
    memblock.memory.max = INIT_MEMBLOCK_REGIONS;
    memblock.memory.total_size = 0;
    INIT_LIST_HEAD(&memblock.memory.regions);

    memblock.reserved.cnt = 0;
    memblock.reserved.max = INIT_MEMBLOCK_REGIONS;
    memblock.reserved.total_size = 0;
    INIT_LIST_HEAD(&memblock.reserved.regions);
}

int memblock_add(phys_addr_t base, size_t size) {
    struct memblock_region* region = alloc_region();
    if (!region) {
        return -1;
    }
    region->base = base;
    region->size = size;
    region->flags = MEMBLOCK_NONE;

    list_add_tail(&memblock.memory.regions, &region->node);

    memblock.memory.cnt++;

    memblock_sort_one(&memblock.memory, region);
    memblock_merge_regions(&memblock.memory, region);

    memblock.memory.total_size += size;
    return 0;
}

int memblock_reserve(phys_addr_t base, size_t size) {
    struct memblock_region* region = alloc_region();
    if (!region) {
        return -1;
    }
    region->base = base;
    region->size = size;
    region->flags = MEMBLOCK_NONE;

    list_add_tail(&memblock.reserved.regions, &region->node);

    memblock.reserved.cnt++;

    memblock_sort_one(&memblock.reserved, region);
    memblock_merge_regions(&memblock.reserved, region);

    memblock.reserved.total_size += size;
    return 0;
}

int memblock_remove(phys_addr_t base, size_t size) {
    struct memblock_type *type = &memblock.memory;
    struct memblock_region *pos = NULL, *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            phys_addr_t pos_end = pos->base + pos->size;
            phys_addr_t end = base + size;

            phys_addr_t left_base =  pos->base;
            phys_addr_t left_end = base < pos_end ? base : pos_end;
            size_t left_size = left_end - left_base;            

            phys_addr_t right_base = end < pos_end ? end : pos_end;
            phys_addr_t right_end = pos_end;
            size_t right_size = right_end - right_base;
            

            struct memblock_region* new_region = NULL;
            int flags = pos->flags;
            list_del(&pos->node);
            free_region(pos);

            if (left_size >0) {
                new_region = alloc_region();
                new_region->base = left_base;
                new_region->size = left_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                memblock_merge_regions(type, new_region);
            }
            
            if (right_size >0) {
                new_region = alloc_region();
                new_region->base = right_base;
                new_region->size = right_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                memblock_merge_regions(type, new_region);
            }
            memblock_sort(type);
        }
    }
    return 0;
}

void memblock_mark_nomap(phys_addr_t base, size_t size) {
    struct memblock_type *type = &memblock.memory;
    struct memblock_region *pos = NULL, *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            phys_addr_t pos_end = pos->base + pos->size;
            phys_addr_t end = base + size;

            phys_addr_t left_base =  pos->base;
            phys_addr_t left_end = base < pos_end ? base : pos_end;
            size_t left_size = left_end - left_base;            

            phys_addr_t mid_base = base > pos->base ? base : pos->base;
            phys_addr_t mid_end = end < pos_end ? end : pos_end;
            size_t mid_size = mid_end - mid_base;

            phys_addr_t right_base = end < pos_end ? end : pos_end;
            phys_addr_t right_end = pos_end;
            size_t right_size = right_end - right_base;
            

            struct memblock_region* new_region = NULL;
            int flags = pos->flags;
            list_del(&pos->node);
            free_region(pos);

            if (left_size >0) {
                new_region = alloc_region();
                new_region->base = left_base;
                new_region->size = left_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                memblock_merge_regions(type, new_region);
            }
            if (mid_size >0) {
                new_region = alloc_region();
                new_region->base = mid_base;
                new_region->size = mid_size;
                new_region->flags = flags | MEMBLOCK_NOMAP;
                list_add(&type->regions, &new_region->node);
                memblock_merge_regions(type, new_region);
            }
            if (right_size >0) {
                new_region = alloc_region();
                new_region->base = right_base;
                new_region->size = right_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                memblock_merge_regions(type, new_region);
            }
            memblock_sort(type);
        }
    }
}

void memblock_mark_reusable(phys_addr_t base, size_t size) {
    
 
}

void *memblock_alloc(size_t size, int align) {
    struct memblock_region *pos = NULL;
    struct memblock_type *type = &memblock.memory;
    list_for_each_entry(pos, &type->regions, struct memblock_region, node) {
        if (pos->size >= size && !(IS_NOMAP(pos->flags)) ) {
            phys_addr_t aligned_base = ALIGN_UP(pos->base, align);
            if (pos->size - (aligned_base - pos->base) >= size) {
                memblock_remove(aligned_base, size);
                memblock_reserve(aligned_base, size);
                return (void*)aligned_base;
            }
        }
    }
    return NULL;
}

uint64_t memblock_phys_total(void) {
    // uint64_t total = 0;
    // for (int i = 0; i < memblock.memory.cnt; i++) {
    //     total += memblock.memory.regions[i].size;
    // }
    // return total;
}

void print_nomap_regions(void) {
    struct memblock_type *type = &memblock.memory;
    struct memblock_region *pos = NULL, *tmp = NULL;
    printk("Nomap Regions:\n");
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (pos->flags & MEMBLOCK_NOMAP) {
            printk("  Region %d: Start: %xu, Size: %xu\n", pos->__idx,
                   pos->base,
                   pos->size);
        }
    }

    type = &memblock.reserved;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (pos->flags & MEMBLOCK_NOMAP) {
            printk("  Region %d: Start: %xu, Size: %xu\n", pos->__idx,
                   pos->base,
                   pos->size);
        }
    }
}

void memblock_dump(void) {
    printk("Memory Regions:\n");
    struct memblock_region *region = NULL;
    list_for_each_entry(region, &memblock.memory.regions, struct memblock_region, node) {
        printk("  Region %d: Start: %xu, Size: %xu Nomap:%xu\n", region->__idx, region->base, region->size, region->flags & MEMBLOCK_NOMAP);
    }
    printk("end\n");

    print_nomap_regions();
    printk("Reserved Regions:\n");
    list_for_each_entry(region, &memblock.reserved.regions, struct memblock_region, node) {
        printk("  Region %d: Start: %xu, Size: %xu\n", region->__idx,
               region->base,
               region->size);
    }
}