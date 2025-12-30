/**
 * @FilePath: /vboot/page_alloc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:26:52
 * @LastEditTime: 2025-09-18 00:01:17
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_EARLY_MALLOC_H
#define __KERNEL_EARLY_MALLOC_H

#include <os/types.h>

#define EARLY_MALLOC_SIZE 4*SIZE_4K  // 4KB

extern phys_addr_t early_malloc_start;

extern void early_malloc_init();
extern void *early_malloc(size_t size);

#endif
