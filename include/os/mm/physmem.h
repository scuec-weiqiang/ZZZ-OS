/**
 * @FilePath: /ZZZ-OS/include/os/mm/physmem.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-20 20:57:29
 * @LastEditTime: 2025-11-20 21:33:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PHYSMEM_H_
#define __KERNEL_PHYSMEM_H_

#include <os/types.h>
#include <os/pfn.h>
#include <os/list.h>

#define _PAGE_RESERVED_BIT  (1U<<0)

struct page {
    uint32_t flags;
    uint16_t order;
    uint16_t _pad;
    uint32_t refcount;
    struct list_head link;
};

extern struct page *mem_map;   // base of page array
extern pfn_t total_pages;
extern pfn_t first_pfn;
extern pfn_t last_pfn;

void physmem_init(void);
struct page *pfn_to_page(pfn_t pfn);
pfn_t page_to_pfn(struct page *p);

#endif