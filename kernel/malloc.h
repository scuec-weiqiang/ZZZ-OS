/**
 * @FilePath: /ZZZ/kernel/malloc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:26:52
 * @LastEditTime: 2025-09-20 16:58:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include "types.h"
#include "mm.h"

#define RESERVED_PAGE_NUM           8
#define RESERVED_PAGE_SIZE          RESERVED_PAGE_NUM*PAGE_SIZE 

extern void malloc_init();
extern void* page_alloc(size_t npages);
extern void* malloc(size_t size);
extern void free(void* p);
extern u64 get_remain_mem();

#endif
