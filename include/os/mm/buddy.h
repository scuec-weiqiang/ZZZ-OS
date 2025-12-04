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

extern struct free_area free_area[MAX_ORDER];

extern void buddy_init(void);
extern struct page* alloc_pages(unsigned int order);
extern void free_pages(struct page *page);
extern void* alloc_pages_kva(size_t npages);
extern void free_pages_kva(void *kaddr);
extern void buddy_test(void);
extern void check_free_area(void);
extern void buddy_dump(void);

#endif /* __KERNEL_BUDDY_H__ */