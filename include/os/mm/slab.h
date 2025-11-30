/**
 * @FilePath: /ZZZ-OS/include/os/mm/slab.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-28 16:10:33
 * @LastEditTime: 2025-11-28 17:12:49
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_SLAB_H__
#define __KERNEL_SLAB_H__

#include <os/types.h>
#include <os/bitmap.h>
#include <asm/spinlock.h>

struct page;

struct kmem_cache {
    const char *name;
    size_t object_size;
    size_t alloc_size;
    unsigned int objects_per_slab;
    struct list_head full_slabs;
    struct list_head partial_slabs;
    struct list_head free_slabs;
    spinlock_t lock;
    unsigned long total_objects;
    unsigned long total_slabs;
};

struct slab {
    struct list_head list;   // 挂在 cache 的 partial/full/free 链表上
    struct kmem_cache *parent;
    void *addr;              // slab 起始 KVA 地址 (page aligned)
    bitmap_t *bitmap;        // 或者 freelist pointer
    unsigned int free_cnt;   // 空闲对象数量
    unsigned int inuse;      // in-use objects
    struct page *page;       // 如果 slab 是单页，则存 page 指针（便于回收）
};

#endif /* __KERNEL_SLAB_H__ */