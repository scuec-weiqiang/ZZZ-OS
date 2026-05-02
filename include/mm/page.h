/**
 * @FilePath: /ZZZ-OS/include/os/mm/page.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 16:38:00
 * @LastEditTime: 2025-12-03 16:17:04
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PAGE_H__
#define __KERNEL_PAGE_H__

#include <os/types.h>  
#include <os/list.h>
#include <os/lru.h>
#include <os/spinlock.h>
#include <os/pfn.h>

#define PAGE_FREE        (0U<<0)
#define PAGE_RESERVED    (1U<<0)

#define PAGE_LOCKED      (1U<<8)
#define PAGE_UPTODATE    (1U<<9)
#define PAGE_DIRTY       (1U<<10)
#define PAGE_WRITEBACK   (1U<<11)

struct address_space;

struct page {
    u32 flags;
    u32 refcount;
    spinlock_t lock;
    struct address_space *mapping;
    uintptr_t index;
    void *private;
    struct lru_node cache_lru_node;
    union {
        struct {  // 对应 buddy allocator
            unsigned int order;
            struct list_head buddy_node;
            struct page *next;
        };
    };
    union {
        struct slab *slab;  // 如果该页被 slab allocator 使用，则指向所属 slab
    };
};

extern struct page* phys_to_page(phys_addr_t phys_addr);
extern phys_addr_t page_to_phys(struct page *page);
extern void *page_address(struct page *page);
extern struct page* address_page(void *va);

static inline int page_test_flag(struct page *page, u32 flag)
{
    return page != NULL && (page->flags & flag) != 0;
}

static inline void page_set_flag(struct page *page, u32 flag)
{
    if (page != NULL) {
        page->flags |= flag;
    }
}

static inline void page_clear_flag(struct page *page, u32 flag)
{
    if (page != NULL) {
        page->flags &= ~flag;
    }
}

#endif /* __KERNEL_PAGE_H__ */
