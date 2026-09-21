/**
 * @FilePath: /ZZZ-OS/include/os/mm/buddy.h
 * @Description:    
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 15:14:10
 * @LastEditTime: 2025-12-03 16:59:44
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_BUDDY_H__
#define __KERNEL_BUDDY_H__

#include <os/list.h>

#define MAX_ORDER 11   // 2^11 = 2048 pages = 8MB，足够

struct free_area {
    struct list_head free_list;
    unsigned long nr_free;
};

struct pcp_stats {
    /* refill/drain 按批次计数，不是页数。 */
    u64 hits, misses, refills, drains;
    u32 count, high, batch;
};

int pcp_get_stats(int cpu, struct pcp_stats *stats);
/* 排空所有 CPU 的 PCP 后更新参数，各 CPU 分别加锁。 */
int pcp_configure(u32 high, u32 batch);

struct buddy_memory_stats {
    u64 total_pages;
    u64 managed_pages;
    u64 reserved_pages;
    u64 used_pages;
    u64 buddy_free_pages;
    u64 pcp_free_pages;
    u64 slab_pages;
    u64 pagecache_pages;
};

struct buddy_fragmentation_stats {
    u64 free_blocks[MAX_ORDER];
    u64 buddy_free_pages;
    u64 pcp_free_pages;
    int largest_free_order;
};

extern struct free_area free_area[MAX_ORDER];

extern void buddy_init(void);
extern struct page* alloc_pages(unsigned int order);
extern void free_pages(struct page *page);
extern void* alloc_pages_kva(size_t npages);
extern void free_pages_kva(void *kaddr);
extern void buddy_test(void);
extern void check_free_area(void);
extern void buddy_dump(void);
extern int buddy_get_memory_stats(struct buddy_memory_stats *stats);
extern int buddy_get_fragmentation_stats(
    struct buddy_fragmentation_stats *stats);

#endif /* __KERNEL_BUDDY_H__ */
