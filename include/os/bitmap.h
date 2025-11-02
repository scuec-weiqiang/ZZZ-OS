/**
 * @FilePath: /vboot/lib/os/bitmap.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 17:54:44
 * @LastEditTime: 2025-09-17 23:49:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BITMAP_H    
#define BITMAP_H

#include <os/types.h>

#define BITMAP_ARR(x) ((char*)(x)+sizeof(size_t))

struct bitmap* bitmap_create(size_t size);
void bitmap_destory(struct bitmap *bm);

int bitmap_set_bit(struct bitmap *bm, uint64_t index);
int bitmap_clear_bit(struct bitmap *bm, uint64_t index);
int bitmap_test_bit(struct bitmap *bm, uint64_t index);
size_t  bitmap_get_size(struct bitmap *bm);
size_t  bitmap_update_size(struct bitmap *bm,uint64_t size);
size_t  bitmap_get_bytes_num(struct bitmap *bm);
size_t  bitmap_get_size_in_bytes(struct bitmap *bm);
int bitmap_scan_0(struct bitmap *bm);

#endif