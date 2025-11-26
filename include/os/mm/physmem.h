/**
 * @FilePath: /ZZZ-OS/include/os/mm/physmem.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-20 20:57:29
 * @LastEditTime: 2025-11-25 21:54:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_PHYSMEM_H_
#define __KERNEL_PHYSMEM_H_

#include <os/types.h>
#include <os/pfn.h>
#include <os/mm/page.h>

extern struct page* mem_map;   // base of page array
extern pfn_t total_pages;
extern pfn_t first_pfn;
extern pfn_t last_pfn;

extern void physmem_init(void);
extern struct page *pfn_to_page(pfn_t pfn);
extern pfn_t page_to_pfn(struct page *p);
extern struct page* phys_to_page(phys_addr_t phys_addr);
extern phys_addr_t page_to_phys(struct page *page);

#endif