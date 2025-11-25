/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/mm/buddy.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 15:14:03
 * @LastEditTime: 2025-11-25 23:14:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/mm/buddy.h>
#include <os/mm/physmem.h>
#include <os/mm/memblock.h>
#include <os/printk.h>
#include <os/rand.h>

#define list_to_page(ptr)               list_entry((ptr), struct page, buddy_node)
#define get_first_page(order)           list_to_page(free_area[(order)].free_list.next)
#define get_buddy_page(page, order)     pfn_to_page( page_to_pfn((page)) ^ (1 << (order)) )

/*  
    这两个宏只涉及free链表的添加、删除以及计数更新
    不负责page的状态更新，且不对添加与删除的合法性进行检查，
    谨慎使用 
*/
#define add_to_free_area(page, order) \
    __PROTECT ( \
        INIT_LIST_HEAD(&page->buddy_node); \
        list_add_tail(&free_area[order].free_list, &page->buddy_node); \
        free_area[order].nr_free++; \
    )
#define remove_from_free_area(page, order) \
    __PROTECT ( \
        list_del(&page->buddy_node); \
        free_area[order].nr_free--; \
    )


struct free_area free_area[MAX_ORDER]= {0};

void try_merge_order(unsigned int order) {
    if (order >= MAX_ORDER - 1) {
        return;
    }

    struct list_head *curr_list = &free_area[order].free_list;
    struct list_head *next_list = &free_area[order + 1].free_list;

    struct page *page, *n;
    list_for_each_entry_safe(page, n, curr_list, struct page, buddy_node) {
        struct page *buddy_page = get_buddy_page(page, order);
        if (buddy_page && buddy_page->flags == PAGE_FREE && buddy_page->order == order) {
            // 更新下一个节点，防止后续访问出错
            n = list_to_page(n->buddy_node.next);

            // 从当前 order 链表中移除
            remove_from_free_area(page, order);
            remove_from_free_area(buddy_page, order);

            // 合并后加入到下一个 order 链表
            struct page *merged_page = page;
            merged_page->order = order + 1;
            merged_page->flags = PAGE_FREE;
            add_to_free_area(merged_page, order+1);
        }
    }
}

struct page* alloc_pages(unsigned int order) {
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
        buddy_page->flags = PAGE_FREE;
        add_to_free_area(buddy_page, new_order);
        page->order = new_order;
        page->flags = PAGE_FREE;
        add_to_free_area(page, new_order);

        current--;
    }

    do_alloc:
    page = get_first_page(current);
    remove_from_free_area(page, order);
    page->flags = PAGE_RESERVED;  // 标记为已分配
    return page;
}

void free_pages(struct page *page) {
    unsigned int order = page->order;
    if (order >= MAX_ORDER) {
        return;
    }

    struct page *curr_page = page;

    while (order < MAX_ORDER - 1) {
        struct page *buddy_page = get_buddy_page(curr_page, order);
        if (buddy_page && buddy_page->flags == PAGE_FREE && buddy_page->order == order) {
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
    curr_page->flags = PAGE_FREE;
    add_to_free_area(curr_page, order);
}

void buddy_init(void) {
    for (unsigned int order = 0; order < MAX_ORDER; order++) {
        INIT_LIST_HEAD(&free_area[order].free_list);
        free_area[order].nr_free = 0;
    }

    struct memblock_region *pos = NULL;
    list_for_each_entry(pos, &memblock.memory.regions, struct memblock_region, node) {
        phys_addr_t base = pos->base;
        phys_addr_t end  = pos->base + pos->size;

        for (phys_addr_t addr = base; addr < end; addr += PAGE_SIZE) {
            if (memblock_is_reserved(addr, PAGE_SIZE)) {
                continue;
            }
            struct page *page = phys_to_page(addr);
            page->order = 0;
            page->flags = PAGE_FREE;
            INIT_LIST_HEAD(&page->buddy_node);
            list_add_tail(&free_area[0].free_list, &page->buddy_node);
            free_area[0].nr_free++;
        }
    }

    for (unsigned int order = 0; order < MAX_ORDER - 1; order++) {
        try_merge_order(order);
    }
}

void buddy_dump(void) {
    printk("Buddy Allocator State:\n");
    for (unsigned int order = 0; order < MAX_ORDER; order++) {
        printk("Order %u: Free blocks: %lu\n", order, free_area[order].nr_free);
    }
    printk("End of Buddy Allocator State.\n");
}

void check_free_area(void)
{
    printk("=== free_area 状态 ===\n");

    for (int order = 0; order < MAX_ORDER; order++) {
        int cnt = 0;
        struct page *p;

        list_for_each_entry(p, &free_area[order].free_list, struct page, buddy_node) {
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

void buddy_test(void)
{
    printk("=== Buddy Allocator Test START ===\n");

    check_free_area();

    printk("\n[1] 测试 alloc/free order=0\n");
    struct page *p0 = alloc_pages(0);
    printk("alloc order0: pfn=%d\n", page_to_pfn(p0));
    check_free_area();

    free_pages(p0);
    printk("free order0\n");
    check_free_area();

    printk("\n[2] 测试拆分 split（alloc order1）\n");
    struct page *p1 = alloc_pages(1);
    printk("alloc order1: pfn=%d\n", page_to_pfn(p1));
    check_free_area();

    free_pages(p1);
    printk("free order1\n");
    check_free_area();

    printk("\n[3] 测试连续申请多个 order0\n");

    struct page *arr[10];
    for (int i = 0; i < 10; i++) {
        arr[i] = alloc_pages(0);
        printk("alloc0[%d] = pfn %d\n", i, page_to_pfn(arr[i]));
    }
    check_free_area();

    for (int i = 0; i < 10; i++) {
        free_pages(arr[i]);
    }
    printk("释放所有 order0\n");
    check_free_area();

    printk("\n[4] 随机压力测试 1000 次\n");
    #define TSIZE 256
    struct page *rand_pages[TSIZE];
    memset(rand_pages, 0, sizeof(rand_pages));

    for (int i = 0; i < 1000; i++) {
        int idx = rand() % TSIZE;

        if (rand_pages[idx] == NULL) {
            int order = rand() % 4;  // 随机 order 0~3
            rand_pages[idx] = alloc_pages(order);
            // 有可能因为内存不足返回 NULL，这不是错误
        } else {
            int order = rand_pages[idx]->order;
            free_pages(rand_pages[idx]);
            rand_pages[idx] = NULL;
        }
    }

    // 把剩下的全部释放
    for (int i = 0; i < TSIZE; i++) {
        if (rand_pages[i]) {
            free_pages(rand_pages[i]);
        }
    }

    check_free_area();

    printk("=== Buddy Allocator Test END ===\n");
}

