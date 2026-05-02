/**
 * @FilePath: /ZZZ-OS/mm/kmalloc.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-12-03 15:42:16
 * @LastEditTime: 2025-12-03 18:26:13
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/printk.h>
#include <mm/early_malloc.h>
#include <mm/memblock.h>
#include <mm/physmem.h>
#include <mm/buddy.h>
#include <mm/slab.h>
#include <os/kva.h>
#include <os/utils.h>
#include <os/string.h>

enum alloc_state {
    EARLY_ALLOC,
    MEMBLOCK_ALLOC,
    NORMAL_ALLOC
};
static enum alloc_state alloc_state = EARLY_ALLOC;

void update_alloc_state() {
    alloc_state++;
    if (alloc_state > NORMAL_ALLOC) {
        alloc_state = NORMAL_ALLOC;
    }
}

/***************************************************************
 * @description:
 * @return {*}
 ***************************************************************/
void kmalloc_init() {
    physmem_init();
    buddy_init();
    slab_init();
    update_alloc_state();
}

/***************************************************************
 * @description:
 * @param {u32} npages [in/out]:
 * @return {*}
 ***************************************************************/
void *page_alloc(size_t npages) {
    switch (alloc_state) {
    case EARLY_ALLOC:
        printk("page_alloc: early alloc no support page alloc\n");
        return NULL;

    case MEMBLOCK_ALLOC:
        return memblock_alloc(npages * PAGE_SIZE, npages*PAGE_SIZE);
        
    case NORMAL_ALLOC:
        return alloc_pages_kva(npages);
    default:
        return NULL;
    }
    
}


/***************************************************************
 * @description:
 * @param {void*} p [in/out]:
 * @return {*}
 ***************************************************************/
void kfree(void *p) {
    if (p == NULL) {
        return;
    }
    switch (alloc_state) {
    case EARLY_ALLOC:
        return;
    case MEMBLOCK_ALLOC:
        memblock_free(KERNEL_PA(p));
        return;
    case NORMAL_ALLOC:
        __kfree(p);
        return;
    }
}

void *kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    switch (alloc_state) {
    case EARLY_ALLOC:
        // printk("kmalloc: early alloc\n");
        return early_malloc(size);
    case MEMBLOCK_ALLOC:
        return memblock_alloc(size,8);
    case NORMAL_ALLOC:
        return __kmalloc(size);
    }
    return NULL;
}

void *kzalloc(size_t size) {
    void *p = kmalloc(size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}
