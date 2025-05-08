/**
 * @FilePath: /ZZZ/kernel/page.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:26:52
 * @LastEditTime: 2025-05-09 02:42:15
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _PAGE_H
#define _PAGE_H

#include "types.h"

extern void page_init();
extern void* page_alloc(uint64_t npages);
extern void page_free(void* p);

#endif
