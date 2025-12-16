/**
 * @FilePath: /ZZZ-OS/mm/mm.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-12-05 18:26:29
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "os/pfn.h"
#include <os/mm/pgtbl_types.h>
#include <os/mm/symbols.h>
#include <drivers/virtio.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/mm/page.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/kva.h>
#include <os/mm/early_malloc.h>
#include <os/mm/vma_flags.h>
#include <os/mm/pgtbl.h>

static int highest_possible_level(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size) {
    int level = pgtbl->features->levels - 1; // 默认使用最小页大小
    for (int i = pgtbl->features->levels - 1; i >= 0; i--) {
        size_t page_size = pgtbl->features->level[i].page_size;
        if ((vaddr % page_size == 0) && (paddr % page_size == 0) && (size >= page_size)) {
            level = i;
            break;
        }
    }
    return level;
}

int map_range(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size, vma_flags_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(vaddr % PAGE_SIZE == 0, "vaddr is not page aligned", return -1;);
    CHECK(paddr % PAGE_SIZE == 0, "paddr is not page aligned", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);

    uintptr_t va = vaddr;
    uintptr_t pa = paddr;
    uintptr_t end = vaddr + size;

    while (va < end) {
        int level = highest_possible_level(pgtbl, va, pa, end - va);
        int map_size = pgtbl_level_page_size(pgtbl, level);
        if (map_one(pgtbl, va, pa, level, flags) < 0) {
            return -1;
        }

        va += map_size;
        pa += map_size;
    }

    pgtbl_flush();
    return 0;
}

int unmap_range(pgtable_t *pgtbl, virt_addr_t va, size_t size) {
    return unmap_one(pgtbl, va);
}

void build_kernel_mapping(pgtable_t *pgtbl) {
    // 映射内核代码段，数据段，栈以及堆的保留页到虚拟地址空间
    map_range(pgtbl, (uintptr_t)text_start, (uintptr_t)KERNEL_PA(text_start), (size_t)text_size, VMA_R | VMA_X);
    map_range(pgtbl, (uintptr_t)rodata_start, (uintptr_t)KERNEL_PA(rodata_start), (size_t)rodata_size, VMA_R);
    map_range(pgtbl, (uintptr_t)data_start, (uintptr_t)KERNEL_PA(data_start), (size_t)data_size, VMA_R | VMA_W);
    map_range(pgtbl, (uintptr_t)bss_start, (uintptr_t)KERNEL_PA(bss_start), (size_t)bss_size, VMA_R | VMA_W);
    
    map_range(pgtbl, (uintptr_t)initcall_start, (uintptr_t)KERNEL_PA(initcall_start), (size_t)initcall_size, VMA_R | VMA_W | VMA_X);
    map_range(pgtbl, (uintptr_t)exitcall_start, (uintptr_t)KERNEL_PA(exitcall_start), (size_t)exitcall_size , VMA_R | VMA_W | VMA_X);
    map_range(pgtbl, (uintptr_t)irqinitcall_start, (uintptr_t)KERNEL_PA(irqinitcall_start), (size_t)irqinitcall_size, VMA_R | VMA_W | VMA_X);
    map_range(pgtbl, (uintptr_t)irqexitcall_start, (uintptr_t)KERNEL_PA(irqexitcall_start), (size_t)irqexitcall_size , VMA_R | VMA_W | VMA_X);
    map_range(pgtbl, (uintptr_t)early_stack_start, (uintptr_t)KERNEL_PA(early_stack_start), (size_t)early_stack_size, VMA_R | VMA_W);

    // map_range(pgtbl, kernel_start, KERNEL_PA(kernel_start), kernel_size, VMA_R | VMA_W | VMA_X);
    map_range(pgtbl, (uintptr_t)heap_start, (uintptr_t)KERNEL_PA(heap_start), (size_t)heap_size, VMA_R | VMA_W);
    map_range(pgtbl, (uintptr_t)stack_start, (uintptr_t)KERNEL_PA(stack_start), (size_t)stack_size * 2, VMA_R | VMA_W);

    map_range(pgtbl, KERNEL_VA(early_malloc_start), early_stack_start, early_stack_size, VMA_R | VMA_W );
    map_range(pgtbl, (uintptr_t)VIRTIO_MMIO_BASE, (uintptr_t)VIRTIO_MMIO_BASE, PAGE_SIZE, VMA_R | VMA_W);
    map_range(pgtbl, (uintptr_t)0x10000000, (uintptr_t)0x10000000, PAGE_SIZE, VMA_R | VMA_W);

}

void kernel_page_table_init() {
    kernel_pgtbl = new_pgtbl("kernel_pgtbl");
    if (kernel_pgtbl == NULL)
        return;
    build_kernel_mapping(kernel_pgtbl);

    pgtbl_switch_to(kernel_pgtbl);
    pgtbl_flush();
    printk("kernel page init success!\n");
}

int copyin(pgtable_t *pagetable, char *dst, uintptr_t src_va, size_t len) {
    size_t n = 0;
    while (n < len) {
        uintptr_t src = pgtbl_walk(pagetable, src_va);
        if (src == 0) {
            return -1;
        }
        size_t offset = src_va % PAGE_SIZE;
        size_t to_copy = PAGE_SIZE - offset;
        if (to_copy > len - n) {
            to_copy = len - n;
        }

        memcpy(dst + n, (char *)(src + offset), to_copy);

        n += to_copy;
        src_va += to_copy;
    }
    return n;
}

int copyout(pgtable_t *pagetable, uintptr_t dst_va, char *src, size_t len) {
    size_t n = 0;
    while (n < len) {
        uintptr_t dst = pgtbl_walk(pagetable, dst_va);
        if (dst == 0) {
            return -1;
        }
        size_t offset = dst_va % PAGE_SIZE;
        size_t to_copy = PAGE_SIZE - offset;
        if (to_copy > len - n) {
            to_copy = len - n;
        }

        memcpy((char *)(dst + offset), src + n, to_copy);

        n += to_copy;
        dst_va += to_copy;
    }
    return n;
}