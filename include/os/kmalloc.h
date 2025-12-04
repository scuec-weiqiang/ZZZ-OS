/**
 * @FilePath: /ZZZ-OS/include/os/kmalloc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:26:52
 * @LastEditTime: 2025-12-04 13:33:11
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _MALLOC_H
#define _MALLOC_H

#include <os/types.h>

extern void kmalloc_init();
extern void* page_alloc(size_t npages);
extern void* kmalloc(size_t size);
extern void kfree(void* p);
extern void update_alloc_state();

#endif
