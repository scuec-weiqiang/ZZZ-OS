/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-10-26 16:38:14
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-03 23:44:02
 * @FilePath: /my_code/include/page.h
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef PAGE_H
#define PAGE_H

#include "types.h"

extern void page_init();
extern void* page_alloc(uint64_t npages);
extern void page_free(void* p);

#endif
