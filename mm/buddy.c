/**
 * @FilePath: /ZZZ-OS/mm/buddy.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 15:14:03
 * @LastEditTime: 2025-12-04 13:30:01
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "os/types.h"
#include <mm/buddy.h>
#include <mm/physmem.h>
#include <mm/memblock.h>
#include <os/printk.h>
#include <os/rand.h>
#include <os/string.h>
#include <os/lru.h>
#include <os/utils.h>
#include <os/spinlock.h>
#include <os/of_cpu.h>

struct per_cpu_pages 
{
    spinlock_t lock;
    struct page *head; // 本 CPU 的单页链表；
    unsigned int count; // 缓存了多少页；

    unsigned int high; // 最高缓存多少页，超出的返回给全局buddy
    unsigned int batch; // 满了之后攒多少个

    unsigned long hits;
    unsigned long misses;
    unsigned long refills;
    unsigned long drains;
};

static struct per_cpu_pages pcp[MAX_CPUS];
static u64 managed_pages;

SPINLOCK_DEFINE(global_buddy_lock);

#define list_to_page(ptr)               list_entry((ptr), struct page, buddy_node)
#define get_first_page(order)           list_to_page(free_area[(order)].free_list.next)
#define get_buddy_page(page, order)     pfn_to_page(page_to_pfn((page)) ^ (1 << (order)))

/*  
    空闲链表操作同时维护 PAGE_BUDDY，避免链表状态和 flags 分离。
    调用者仍需保证 page/order 合法并持有 buddy 锁。
*/
#define add_to_free_area(page, order) \
    __PROTECT ( \
        INIT_LIST_HEAD(&page->buddy_node); \
        SetPageBuddy(page); \
        list_add_tail(&free_area[order].free_list, &page->buddy_node); \
        free_area[order].nr_free++; \
    )
#define remove_from_free_area(page, order) \
    __PROTECT ( \
        list_del(&page->buddy_node); \
        ClearPageBuddy(page); \
        free_area[order].nr_free--; \
    )

static int n_to_order(u32 n) 
{
    unsigned int order = 0;
    unsigned int size = 1;
    while (size < n) {
        size <<= 1;
        order++;
    }
    return order;
}

struct free_area free_area[MAX_ORDER];

void try_merge_order(unsigned int order) 
{
    if (order >= MAX_ORDER - 1) {
        return;
    }

    struct list_head *curr_list = &free_area[order].free_list;

    struct page *page, *n;
    list_for_each_entry_safe(page, n, curr_list, buddy_node) {
        struct page *buddy_page = get_buddy_page(page, order);
        if (buddy_page && PageBuddy(buddy_page) && buddy_page->order == order) {
            // 更新下一个节点，防止后续访问出错
            n = list_to_page(n->buddy_node.next);

            // 从当前 order 链表中移除
            remove_from_free_area(page, order);
            remove_from_free_area(buddy_page, order);

            // 合并后加入到下一个 order 链表
            struct page *merged_page = page < buddy_page ? page : buddy_page;
            merged_page->order = order + 1;
            merged_page->flags = 0;
            add_to_free_area(merged_page, order+1);
        }
    }
}

static void prepare_allocated_page(struct page *page) 
{
    page->refcount = 0;
    page->mapping = NULL;
    page->index = 0;
    page->private = NULL;
    lru_node_reset(&page->cache_lru_node);
    page->slab = NULL;
}

/* 调用者已经持有 global_buddy_lock */
static struct page *__buddy_alloc_locked(unsigned int order) 
{
    if (order >= MAX_ORDER) {
        return NULL;
    }

    int find = 0;
    int current;
    for (current = order; current < MAX_ORDER; current++) {
        if (free_area[current].nr_free > 0) {
            find = 1;
            break;
        }
    }

    if (!find) {
        return NULL;
    }

    // 找到合适阶的空闲块,直接分配
    if (current == order) {
        goto do_alloc;
    }

    struct page *page = NULL;
    // 没有合适的，从更高阶块中拆分后再分配
    while (current > order) {
        page = get_first_page(current);
        remove_from_free_area(page, current);

        // 拆分成两个较小的块
        unsigned int new_order = current - 1;
        struct page *buddy_page = get_buddy_page(page, new_order);
        // 将拆分出的两个块加入到较小阶的空闲链表中
        buddy_page->order = new_order;
        buddy_page->flags = 0;
        buddy_page->slab = NULL;
        add_to_free_area(buddy_page, new_order);
        page->order = new_order;
        page->flags = 0;
        page->slab = NULL;
        add_to_free_area(page, new_order);

        current--;
    }

    do_alloc:
    page = get_first_page(current);
    remove_from_free_area(page, order);
    return page;
}

/* 调用者必须已经持有 global_buddy_lock */
static void __buddy_free_locked(struct page *page) 
{
    unsigned int order = page->order;
    if (order >= MAX_ORDER) {
        return;
    }

    if (PageReserved(page) || PageBuddy(page) || PagePcp(page)) {
        return;
    }

    struct page *curr_page = page;

    while (order < MAX_ORDER - 1) {
        struct page *buddy_page = get_buddy_page(curr_page, order);
        if (buddy_page && PageBuddy(buddy_page) && buddy_page->order == order) {
            // 从当前 order 链表中移除伙伴页
            remove_from_free_area(buddy_page, order);

            // 合并
            if (curr_page > buddy_page) {
                curr_page = buddy_page;
            }
            order++;
        } else {
            break;
        }
    }

    curr_page->order = order;
    curr_page->flags = 0;
    curr_page->refcount = 0;
    curr_page->mapping = NULL;
    curr_page->index = 0;
    curr_page->private = NULL;
    curr_page->next = NULL;
    lru_node_reset(&curr_page->cache_lru_node);
    curr_page->slab = NULL;
    add_to_free_area(curr_page, order);
}

/* 调用这个函数前必须已持有当前 PCP 的锁 */
static unsigned int pcp_refill(struct per_cpu_pages *ppcp)
{
    unsigned int nr = 0;
    int flags;

    flags = spin_lock_irqsave(&global_buddy_lock);

    while (nr < ppcp->batch) {
        struct page *page = __buddy_alloc_locked(0);

        if (page == NULL)
            break;

        SetPagePcp(page);
        page->next = ppcp->head;
        ppcp->head = page;
        ppcp->count++;
        nr++;
    }

    spin_unlock_irqrestore(&global_buddy_lock, flags);

    if (nr != 0)
        pcp->refills++;

    return nr;
}

/* 把pcp里的page还给buddy，调用这个函数前必须已持有当前 PCP 的锁 */
static unsigned int pcp_drain(struct per_cpu_pages *pcp,
                              unsigned int nr_to_drain)
{
    unsigned int nr = 0;
    int flags;

    flags = spin_lock_irqsave(&global_buddy_lock);

    while (nr < nr_to_drain && pcp->head != NULL) {
        struct page *page = pcp->head;

        pcp->head = page->next;
        pcp->count--;

        page->next = NULL;
        ClearPagePcp(page);
        page->order = 0;

        __buddy_free_locked(page);
        nr++;
    }

    spin_unlock_irqrestore(&global_buddy_lock, flags);

    if (nr != 0)
        pcp->drains++;

    return nr;
}

static struct page *__pcp_pop(struct per_cpu_pages *pcp)
{
    struct page *page = pcp->head;

    if (page == NULL)
        return NULL;

    pcp->head = page->next;
    pcp->count--;

    page->next = NULL;
    ClearPagePcp(page);       /* 现在变成普通 allocated page */
    return page;
}

static void __pcp_push(struct per_cpu_pages *pcp, struct page *page)
{
    page->order = 0;
    SetPagePcp(page);

    page->next = pcp->head;
    pcp->head = page;
    pcp->count++;
}

static struct page *buddy_alloc(unsigned int order)
{
    int flags;
    struct page *page;

    flags = spin_lock_irqsave(&global_buddy_lock);
    page = __buddy_alloc_locked(order);
    spin_unlock_irqrestore(&global_buddy_lock, flags);

    return page;
}

static void buddy_free(struct page *page) 
{
    int flags;
    flags = spin_lock_irqsave(&global_buddy_lock);
    __buddy_free_locked(page);
    spin_unlock_irqrestore(&global_buddy_lock, flags);
}

struct page *alloc_pages(unsigned int order)
{
    struct per_cpu_pages *ppcp;
    struct page *page;
    int flags;

    // 只有分配一页的请求才走per cpu，否则走全局
    if (order != 0)
        return buddy_alloc(order);

    ppcp = &pcp[get_cpuid()];

    flags = spin_lock_irqsave(&ppcp->lock);

    page = __pcp_pop(ppcp);
    if (page == NULL) {
        ppcp->misses++;
        pcp_refill(ppcp);
        page = __pcp_pop(ppcp);
    } else {
        ppcp->hits++;
    }

    spin_unlock_irqrestore(&ppcp->lock, flags);

    if (page != NULL)
        prepare_allocated_page(page);

    return page;
}

void free_pages(struct page *page)
{
    struct per_cpu_pages *ppcp;
    int flags;

    if (page == NULL)
        return;

    if (page->order != 0) {
        buddy_free(page);
        return;
    }

    ppcp = &pcp[get_cpuid()];

    flags = spin_lock_irqsave(&ppcp->lock);

    __pcp_push(ppcp, page);

    if (ppcp->count > ppcp->high)
        pcp_drain(ppcp, ppcp->batch);

    spin_unlock_irqrestore(&ppcp->lock, flags);
}

void* alloc_pages_kva(size_t npages) 
{
    u32 n = next_power_of_two(npages);
    n = n_to_order(n);
    struct page* page = alloc_pages(n);
    if (!page) return NULL;
    return (void*)page_address(page);
}

void free_pages_kva(void *kaddr) 
{
    struct page* page = address_page(kaddr);
    free_pages(page);
}

void buddy_init(void) 
{
    managed_pages = 0;
    for (unsigned int order = 0; order < MAX_ORDER; order++) {
        INIT_LIST_HEAD(&free_area[order].free_list);
        free_area[order].nr_free = 0;
    }

    struct memblock_region *pos = NULL;
    list_for_each_entry(pos, &memblock.memory.region_head.node, node) {
        phys_addr_t base = pos->base;
        phys_addr_t end  = pos->base + pos->size;

        for (phys_addr_t addr = base; addr < end; addr += PAGE_SIZE) {
            if (memblock_is_reserved(addr, PAGE_SIZE)) {
                continue;
            }
            struct page *page = phys_to_page(addr);
            page->order = 0;
            page->flags = 0;
            page->slab = NULL;
            add_to_free_area(page, 0);
            managed_pages++;
        }
    }

    for (unsigned int order = 0; order < MAX_ORDER - 1; order++) {
        try_merge_order(order);
    }

    for (int i = 0; i < MAX_CPUS; i++) {
        spin_lock_init(&pcp[i].lock);
        pcp[i].head = NULL;
        pcp[i].count = 0;
        pcp[i].high = 32;
        pcp[i].batch = 8;
        // pcp[i].hits = 0;
        // pcp[i].misses = 0;
        // pcp[i].refills = 0;
        // pcp[i].drains = 0;
    }

}

int buddy_get_memory_stats(struct buddy_memory_stats *stats)
{
    unsigned long pcp_flags[MAX_CPUS];
    unsigned long buddy_flags;
    u64 free_pages = 0;

    if (!stats)
        return -1;

    memset(stats, 0, sizeof(*stats));

    /* Keep the existing PCP -> Buddy lock order used by allocation paths. */
    for (int cpu = 0; cpu < MAX_CPUS; cpu++)
        pcp_flags[cpu] = spin_lock_irqsave(&pcp[cpu].lock);

    buddy_flags = spin_lock_irqsave(&global_buddy_lock);
    for (u32 order = 0; order < MAX_ORDER; order++)
        stats->buddy_free_pages += free_area[order].nr_free << order;
    for (int cpu = 0; cpu < MAX_CPUS; cpu++)
        stats->pcp_free_pages += pcp[cpu].count;
    spin_unlock_irqrestore(&global_buddy_lock, buddy_flags);

    for (int cpu = MAX_CPUS - 1; cpu >= 0; cpu--)
        spin_unlock_irqrestore(&pcp[cpu].lock, pcp_flags[cpu]);

    /* Include NOMAP/reserved regions so this matches configured RAM size. */
    stats->total_pages = memblock.memory.total_size / PAGE_SIZE;
    stats->managed_pages = managed_pages;
    if (stats->total_pages > stats->managed_pages)
        stats->reserved_pages = stats->total_pages - stats->managed_pages;

    free_pages = stats->buddy_free_pages + stats->pcp_free_pages;
    if (stats->managed_pages > free_pages)
        stats->used_pages = stats->managed_pages - free_pages;

    /* These two values are a diagnostic breakdown of allocated pages. */
    for (pfn_t pfn = first_pfn; pfn < last_pfn; pfn++) {
        struct page *page = pfn_to_page(pfn);

        if (!page || PageReserved(page) || PageBuddy(page) || PagePcp(page))
            continue;
        if (page->slab)
            stats->slab_pages++;
        if (page->mapping)
            stats->pagecache_pages++;
    }

    return 0;
}

int buddy_get_fragmentation_stats(struct buddy_fragmentation_stats *stats)
{
    unsigned long pcp_flags[MAX_CPUS];
    unsigned long buddy_flags;

    if (!stats)
        return -1;

    memset(stats, 0, sizeof(*stats));
    stats->largest_free_order = -1;

    /* Match the PCP -> Buddy lock order used by the allocation paths. */
    for (int cpu = 0; cpu < MAX_CPUS; cpu++)
        pcp_flags[cpu] = spin_lock_irqsave(&pcp[cpu].lock);

    buddy_flags = spin_lock_irqsave(&global_buddy_lock);
    for (u32 order = 0; order < MAX_ORDER; order++) {
        stats->free_blocks[order] = free_area[order].nr_free;
        stats->buddy_free_pages += free_area[order].nr_free << order;
        if (free_area[order].nr_free != 0)
            stats->largest_free_order = order;
    }
    for (int cpu = 0; cpu < MAX_CPUS; cpu++)
        stats->pcp_free_pages += pcp[cpu].count;
    spin_unlock_irqrestore(&global_buddy_lock, buddy_flags);

    for (int cpu = MAX_CPUS - 1; cpu >= 0; cpu--)
        spin_unlock_irqrestore(&pcp[cpu].lock, pcp_flags[cpu]);

    return 0;
}

void buddy_dump(void) 
{
    printk("Buddy Allocator State:\n");
    for (unsigned int order = 0; order < MAX_ORDER; order++) {
        printk("Order %xu: Free blocks: %xu\n", order, free_area[order].nr_free);
    }
    printk("End of Buddy Allocator State.\n");
}

void check_free_area(void) 
{
    printk("=== free_area 状态 ===\n");

    for (int order = 0; order < MAX_ORDER; order++) {
        int cnt = 0;
        struct page *p;

        list_for_each_entry(p, &free_area[order].free_list, buddy_node) {
            cnt++;
        }

        printk("order %d: nr_free=%d, count=%d %s\n",
            order,
            free_area[order].nr_free,
            cnt,
            (cnt == free_area[order].nr_free ? "OK" : "MISMATCH!")
        );
    }
}

// void buddy_test(void) {
//     printk("=== Buddy Allocator Test START ===\n");

//     check_free_area();

//     printk("\n[1] 测试 alloc/kfree order=0\n");
//     struct page *p0 = alloc_pages(0); 
//     printk("alloc order0: pfn=%d\n", page_to_pfn(p0));
//     check_free_area();

//     free_pages(p0);
//     printk("kfree order0\n");
//     check_free_area();

//     printk("\n[2] 测试拆分 split（alloc order1）\n");
//     struct page *p1 = alloc_pages(1);
//     printk("alloc order1: pfn=%d\n", page_to_pfn(p1));
//     check_free_area();

//     free_pages(p1);
//     printk("kfree order1\n");
//     check_free_area();

//     printk("\n[3] 测试连续申请多个 order0\n");

//     struct page *arr[10];
//     for (int i = 0; i < 10; i++) {
//         arr[i] = alloc_pages(0);
//         printk("alloc0[%d] = pfn %d\n", i, page_to_pfn(arr[i]));
//     }
//     check_free_area();

//     for (int i = 0; i < 10; i++) {
//         free_pages(arr[i]);
//     }
//     printk("释放所有 order0\n");
//     check_free_area();

//     printk("\n[4] 随机压力测试 1000 次\n");
//     #define TSIZE 256
//     struct page *rand_pages[TSIZE];
//     memset(rand_pages, 0, sizeof(rand_pages));

//     for (int i = 0; i < 1000; i++) {
//         int idx = rand() % TSIZE;

//         if (rand_pages[idx] == NULL) {
//             int order = rand() % 4;  // 随机 order 0~3
//             rand_pages[idx] = alloc_pages(order);
//             // 有可能因为内存不足返回 NULL，这不是错误
//         } else {
//             free_pages(rand_pages[idx]);
//             rand_pages[idx] = NULL;
//         }
//     }

//     // 把剩下的全部释放
//     for (int i = 0; i < TSIZE; i++) {
//         if (rand_pages[i]) {
//             free_pages(rand_pages[i]);
//         }
//     }

//     check_free_area();

//     printk("=== Buddy Allocator Test END ===\n");
// }
