#include "os/mm/early_malloc.h"
#include <os/mm/symbols.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/types.h>
#include <os/mm/memblock.h>


static phys_addr_t alloc_pos = 0;
static size_t free_size = 0;

phys_addr_t early_malloc_start = 0;

void early_malloc_init() {
    free_size = EARLY_MALLOC_SIZE;                 // 可分配内存的大小
    early_malloc_start = stack_start - free_size; // 可分配内存的起始地址
    alloc_pos = early_stack_start;
}

void *early_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (size > free_size) {
        printk("page_alloc: no enough memory\n");
        return NULL;
    }
    alloc_pos = (alloc_pos + 7) & (~0x7); // 8字节对齐
    void *p = (void *)alloc_pos;
    alloc_pos += size;
    free_size -= size;
    return p;
}

void *early_page_alloc(size_t npages) {
    size_t size = npages * 4096;
    if (size == 0) {
        return NULL;
    }
    if (size > free_size) {
        printk("page_alloc: no enough memory\n");
        return NULL;
    }
    alloc_pos = (alloc_pos + 4095) & (~0xFFF); // 4k字节对齐
    void *p = (void *)alloc_pos;
    alloc_pos += size;
    free_size -= size;
    return p;
}

void early_free(void *ptr) {
    (void)ptr;
}