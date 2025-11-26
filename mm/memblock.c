/**
 * @FilePath: /ZZZ-OS/mm/memblock.c
 * @Description: 
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-14 16:00:57
 * @LastEditTime: 2025-11-25 16:40:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "os/types.h"
#include <os/mm/memblock.h>
#include <os/printk.h>
#include <os/list.h>
#include <os/pfn.h>
#include <os/mm/symbols.h>
#include <os/mm.h>
#include <os/minmax.h>
#include <os/of.h>
#include <os/mm/early_malloc.h>


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

static struct memblock_region* memblock_get(int idx) {
    if (idx < 0 || idx >= INIT_MEMBLOCK_REGIONS) {
        return NULL;
    }
    return &memblock_regions_pool[idx];
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

static unsigned long memblock_addrs_continue(phys_addr_t base1, size_t size1, phys_addr_t base2, size_t size2) {
    return ((base1 + size1 == base2) || (base2 + size2 == base1));
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

            if ((memblock_addrs_overlap(base, size, rgnbase, rgnsize) ||
                memblock_addrs_continue(base, size, rgnbase, rgnsize)) && (pos->flags == region->flags)) {
                phys_addr_t new_base = min(base, rgnbase);
                phys_addr_t new_end = max(base+size, rgnbase+rgnsize);
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

static int memblock_add_region(struct memblock_type *type, phys_addr_t base, size_t size, enum memblock_flags flags) {
    struct memblock_region* region = alloc_region();
    if (!region) {
        return -1;
    }
    region->base = base;
    region->size = size;
    region->flags = flags;

    list_add_tail(&type->regions, &region->node);

    memblock_sort_one(type, region);
    // memblock_merge_regions(type, region);

    type->total_size += size;
    return 0;
}

int memblock_add(phys_addr_t base, size_t size) {
    return memblock_add_region(&memblock.memory, base, size, 0);
}

static int memblock_remove_region(struct memblock_type *type, phys_addr_t base, size_t size) {
    struct memblock_region *pos = NULL, *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            phys_addr_t pos_end = pos->base + pos->size;
            phys_addr_t end = base + size;

            phys_addr_t left_base =  pos->base;
            phys_addr_t left_end = min(base, pos_end);
            size_t left_size = left_end - left_base;            

            phys_addr_t right_base = min(end , pos_end);
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
                // memblock_merge_regions(type, new_region);
            }
            
            if (right_size >0) {
                new_region = alloc_region();
                new_region->base = right_base;
                new_region->size = right_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                // memblock_merge_regions(type, new_region);
            }
            memblock_sort(type);
        }
    }
    return 0;
}

int memblock_remove(phys_addr_t base, size_t size) {
    return memblock_remove_region(&memblock.memory, base, size);
}

int memblock_reserve(phys_addr_t base, size_t size) {
    return memblock_add_region(&memblock.reserved, base, size, 0);
}


void memblock_mark_nomap(phys_addr_t base, size_t size) {
    struct memblock_type *type = &memblock.memory;
    struct memblock_region *pos = NULL, *tmp = NULL;
    list_for_each_entry_safe(pos, tmp, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            phys_addr_t pos_end = pos->base + pos->size;
            phys_addr_t end = base + size;

            phys_addr_t left_base =  pos->base;
            phys_addr_t left_end = min(base, pos_end);
            size_t left_size = left_end - left_base;            

            phys_addr_t mid_base = max(base, pos->base);
            phys_addr_t mid_end = min(end, pos_end);
            size_t mid_size = mid_end - mid_base;

            phys_addr_t right_base = min(end, pos_end);
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
                // memblock_merge_regions(type, new_region);
            }
            if (mid_size >0) {
                new_region = alloc_region();
                new_region->base = mid_base;
                new_region->size = mid_size;
                new_region->flags = flags | MEMBLOCK_NOMAP;
                list_add(&type->regions, &new_region->node);
                // memblock_merge_regions(type, new_region);
            }
            if (right_size >0) {
                new_region = alloc_region();
                new_region->base = right_base;
                new_region->size = right_size;
                new_region->flags = flags;
                list_add(&type->regions, &new_region->node);
                // memblock_merge_regions(type, new_region);
            }
            memblock_sort(type);
        }
    }
}

void memblock_mark_reusable(phys_addr_t base, size_t size) {
}

struct memblock_region* memblock_is_reserved(phys_addr_t base, size_t size) {
    struct memblock_type *type = &memblock.reserved;
    struct memblock_region *pos = NULL, *n = NULL;
    list_for_each_entry_safe(pos, n, &type->regions, struct memblock_region, node) {
        if (memblock_addrs_overlap(base, size, pos->base, pos->size)) {
            return &memblock_regions_pool[pos->__idx];
        }
    }
    return NULL;
}

void *memblock_alloc(size_t size, int align) {
    struct memblock_region *m;
    list_for_each_entry(m, &memblock.memory.regions, struct memblock_region, node) {
        phys_addr_t m_start = m->base;
        phys_addr_t m_end   = m->base + m->size;

        // 生成当前 memory 区间的所有 gap
        phys_addr_t gap_start = m_start;

        struct memblock_region *r;
        list_for_each_entry(r, &memblock.reserved.regions, struct memblock_region, node) {
            phys_addr_t r_start = r->base;
            phys_addr_t r_end   = r->base + r->size;

            // reserved 不在 memory 区间内部的直接忽略
            if (r_end <= m_start || r_start >= m_end)
                continue;

            // 检查 gap_start 到 reserved_start 之间是否可用
            phys_addr_t gap_end = r_start;

            phys_addr_t aligned = ALIGN_UP(gap_start, align);
            if (aligned + size <= gap_end) {
                memblock_reserve(aligned, size);
                // printk("\nmemblock_alloc: allocated at %xu\n", aligned);
                // extern void memblock_dump();
                // memblock_dump();
                return (void*)KERNEL_VA(aligned);
            }
            gap_start = r_end;
        }
        phys_addr_t aligned = ALIGN_UP(gap_start, align);
        if (aligned + size <= m_end) {
            memblock_reserve(aligned, size);
            // printk("memblock_alloc: allocated at %xu\n", aligned);
            // extern void memblock_dump();
            // memblock_dump();
            return (void*)KERNEL_VA(aligned);
        }
    }

    return NULL;
}

void memblock_free(phys_addr_t addr) {
    addr = KERNEL_PA(addr);
    struct memblock_region *pos = NULL;
    list_for_each_entry(pos, &memblock.reserved.regions, struct memblock_region, node) {
        if (addr >= pos->base && addr < pos->base + pos->size) {
            break;
        }
    }
    if (pos != NULL) {
        memblock_remove_region(&memblock.reserved, pos->base, pos->size);
    }
}

uint64_t memblock_phys_total(void) {
    return memblock.memory.total_size;
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
            printk("  Region %d: Start: %xu, Size: %xu", pos->__idx,
                   pos->base,
                   pos->size);
        }
    }
}

#include <os/color.h>
void memblock_dump(void) {
    printk("\n\nMemory Regions:\n");
    struct memblock_region *region = NULL;
    list_for_each_entry(region, &memblock.memory.regions, struct memblock_region, node) {
        printk(GREEN("  Region %d: Start: %xu, Size: %xu Nomap:%xu\n"), region->__idx, region->base, region->size, region->flags & MEMBLOCK_NOMAP);
    }
    // print_nomap_regions();
    printk("Reserved Regions: %xu\n", memblock.reserved.total_size);
    list_for_each_entry(region, &memblock.reserved.regions, struct memblock_region, node) {
        printk(RED("  Region %d: Start: %xu, Size: %xu\n"), region->__idx,
               region->base,
               region->size);
    }
    
    printk("end\n\n");
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
    
    memblock.memory.total_size = 0;
    INIT_LIST_HEAD(&memblock.memory.regions);

    memblock.reserved.total_size = 0;
    INIT_LIST_HEAD(&memblock.reserved.regions);
    of_scan_memory();
	of_scan_reserved_memory();
	memblock_reserve(KERNEL_PA(kernel_start), kernel_size);
    memblock_reserve(early_malloc_start, EARLY_MALLOC_SIZE);
    memblock_dump();

    extern void update_alloc_state();
    update_alloc_state();
}