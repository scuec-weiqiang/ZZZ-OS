/**
 * @FilePath: /ZZZ/kernel/page_alloc.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:26:52
 * @LastEditTime: 2025-05-09 20:50:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _PAGE_ALLOC_H
#define _PAGE_ALLOC_H

#include "types.h"
#include "mm.h"

#define RESERVED_PAGE_NUM           8
#define RESERVED_PAGE_SIZE          RESERVED_PAGE_NUM*PAGE_SIZE 

extern void page_alloc_init();
extern void* page_alloc(uint64_t npages);
extern void page_free(void* p);

#endif
