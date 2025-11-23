/**
 * @FilePath: /ZZZ-OS/include/os/mm/physmem.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-20 20:57:29
 * @LastEditTime: 2025-11-21 16:58:38
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PHYSMEM_H_
#define __KERNEL_PHYSMEM_H_

#include <os/types.h>
#include <os/pfn.h>
#include <os/list.h>
#include <os/hlist.h>
#include <os/lru.h>
#include <asm/spinlock.h>

#define _PAGE_RESERVED_BIT  (1U<<0)

typedef uintptr_t pgoff_t; // page index

struct page
{
    uint32_t flags;
    uint32_t refcount;
    spinlock_t lock;
    union {
        struct {  // 对应 page cache 页
            struct inode *inode;
            pgoff_t index;
            struct lru_node p_lru_cache_node;  // 全局page lru缓存链表节点
            struct hlist_node self_cache_node; // 哈希表节点，用于快速查找inode私有的page缓存
            void *data;
            bool under_io:1;  // 正在读/写磁盘
            bool uptodate:1;  // 内容有效
            bool dirty:1;     // 脏页标志
        };
        
        struct {  // 对应 buddy allocator
            int order;
            struct page *next;
        };
    };

    
};
extern struct page *mem_map;   // base of page array
extern pfn_t total_pages;
extern pfn_t first_pfn;
extern pfn_t last_pfn;

void physmem_init(void);
struct page *pfn_to_page(pfn_t pfn);
pfn_t page_to_pfn(struct page *p);

#endif