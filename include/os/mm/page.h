/**
 * @FilePath: /ZZZ-OS/include/os/mm/page.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 16:38:00
 * @LastEditTime: 2025-11-28 16:43:38
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PAGE_H__
#define __KERNEL_PAGE_H__

#include <os/types.h>  
#include <os/list.h>
#include <asm/spinlock.h>

#define PAGE_FREE      (0U<<0)
#define PAGE_RESERVED  (1U<<0)

struct slab;

struct page {
    uint32_t flags;
    uint32_t refcount;
    spinlock_t lock;
    union {
        struct {  // 对应 buddy allocator
            unsigned int order;
            struct list_head buddy_node;
            struct page *next;
        };
    };
    struct slab *slab;  // 如果该页被 slab allocator 使用，则指向所属 slab
};

#endif /* __KERNEL_PAGE_H__ */