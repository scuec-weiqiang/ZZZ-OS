#include <os/mm/symbols.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/types.h>
#include <os/mm/memblock.h>

static uintptr_t alloc_pos;
static size_t free_size = 0;

void early_malloc_init() {
    free_size = SIZE_1M;                 // 可分配内存的大小
    alloc_pos = stack_start - free_size; // 可分配内存的起始地址
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