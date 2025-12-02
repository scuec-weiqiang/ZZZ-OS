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
    char name[32];
    size_t object_size;
    size_t align;
    unsigned int objects_per_slab;
    unsigned long total_slabs;

    struct list_head full_slabs;
    struct list_head partial_slabs;
    struct list_head free_slabs;

    spinlock_t lock;
};

struct free_obj {
    void *next;
};

struct slab {
    struct list_head list;   // 挂在 cache 的 partial/full/free 链表上
    struct kmem_cache *parent;
    struct free_obj free_object; // 指向下一个空闲对象
    unsigned int inuse;      // in-use objects
};

extern void slab_test();

#endif /* __KERNEL_SLAB_H__ */