/**
 * @FilePath: /ZZZ-OS/mm/mm.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-12-05 18:26:29
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "os/mm/mm_types.h"
#include "os/pfn.h"
#include <os/mm/pgtbl_types.h>
#include <os/mm/symbols.h>
#include <drivers/virtio.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/mm/memblock.h>
#include <os/mm/page.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/kva.h>
#include <os/mm/vma_flags.h>
#include <os/mm/pgtbl.h>
#include <os/mm/vma.h>
#include <os/mm.h>

static int highest_possible_level(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size) {
    if (pgtbl == NULL||pgtbl->features == NULL) {
        return -1;
    }

    if ((pgtbl->features->features & PGTABLE_FEATURE_HUGE_PAGES) == 0) {
        return pgtbl->features->support_levels - 1; // 不支持大页，返回最低层级
    }
    
    int level = pgtbl->features->support_levels - 1; // 默认使用最小页大小
    for (int i = 0; i < pgtbl->features->support_levels ; i++) {
        size_t page_size = pgtbl->features->level[i].page_size;
        if ((vaddr % page_size == 0) && (paddr % page_size == 0) && (size >= page_size)) {
            level = i;
            break;
        }
    }
    return level;
}

int map(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size, vma_flags_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);
    uintptr_t va = ALIGN_DOWN(vaddr, PAGE_SIZE);
    uintptr_t pa = ALIGN_DOWN(paddr, PAGE_SIZE);
    uintptr_t end = va + size;

    while (va < end) {
        int target_level = highest_possible_level(pgtbl, va, pa, end - va);
        int map_size = pgtbl_level_page_size(pgtbl, target_level);
        if (pgtbl_map(pgtbl, va, pa, target_level, flags) < 0) {
            return -1;
        }

        va += map_size;
        pa += map_size;
    }

    pgtbl_flush();
    return 0;
}

int unmap(pgtable_t *pgtbl, virt_addr_t va, size_t size) {
    CHECK(pgtbl != NULL, "mm is NULL", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);
    va = ALIGN_DOWN(va, PAGE_SIZE);
    uintptr_t start = va;
    uintptr_t end = start + size;

    while (va < end) {
        int target_level = highest_possible_level(pgtbl, va, 0, end - va);
        int unmap_size = pgtbl_level_page_size(pgtbl, target_level);
        pgtbl_unmap(pgtbl, va, target_level);

        va += unmap_size;
    }

    pgtbl_flush();
    return 0;
}



struct mm_struct init_mm = {
    .pgdir = NULL,
    .vma_list = {NULL, NULL},
    .vma_count = 0,
};

struct mm_struct *kernel_mm_struct = NULL;
struct mm_struct *current_mm_struct = NULL;

struct mm_struct *mm_create(char *name) {
    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));
    if (!mm) {
        return NULL;
    }
    mm->pgdir = new_pgtbl(name);
    if (!mm->pgdir) {
        kfree(mm);
        return NULL;
    }
    INIT_LIST_HEAD(&mm->vma_list);
    mm->vma_count = 0;
    vma_add(mm, 0, 1, 0); // 添加哨兵节点
    return mm;
}

int do_mmap(struct mm_struct * mm, virt_addr_t vaddr, size_t size, vma_flags_t flags) {
    CHECK(mm != NULL, "mm is NULL", return -1;);

    pgtable_t *pgtbl = mm->pgdir;
    size = ALIGN_UP(size, PAGE_SIZE);
    uintptr_t va = ALIGN_DOWN(vaddr, PAGE_SIZE);
    uintptr_t end = va + size;

    vma_add(mm, va, size, flags);

    pgtbl_flush();
    return 0;
}

int do_munmap(struct mm_struct *mm, virt_addr_t va, size_t size) {
    CHECK(mm != NULL, "mm is NULL", return -1;);

    size = ALIGN_UP(size, PAGE_SIZE);
    va = ALIGN_DOWN(va, PAGE_SIZE);

    vma_delete(mm, va, size);
   
    return 0;
}

static virt_addr_t alloc_mmio_va(size_t size) {
    static virt_addr_t current_mmio_va = KERNEL_MMIO_BASE;
    for (int i = 0; i < kernel_mm_struct->pgdir->features->support_levels; i++) {
        size_t page_size = kernel_mm_struct->pgdir->features->level[i].page_size;
        if (size >= page_size) {
            current_mmio_va = ALIGN_UP(current_mmio_va, page_size);
            break;
        }
    }
    virt_addr_t va = current_mmio_va;
    current_mmio_va += ALIGN_UP(size, PAGE_SIZE);
    return va;
}

void *ioremap(phys_addr_t pa, size_t size) {
    uintptr_t va = alloc_mmio_va(size);
    map(kernel_mm_struct->pgdir, va, pa, size, VMA_R | VMA_W);
    return (void *)va;
}

void iounmap(virt_addr_t va, size_t size) {
    unmap(kernel_mm_struct->pgdir, va, size);
}

void copy_kernel_mapping(struct mm_struct *dest_mm) {
    int kernel_start_index = pgtbl_level_index(kernel_mm_struct->pgdir, 0, KERNEL_VA_BASE);
    int kernel_end_index = PAGE_SIZE - sizeof(pte_t);
    pgtbl_copy(dest_mm->pgdir, kernel_mm_struct->pgdir, 0, kernel_start_index, kernel_end_index - kernel_start_index);
}

void mm_init() {
    kernel_mm_struct = &init_mm;
    kernel_mm_struct->pgdir = new_pgtbl("kernel_pgtbl");
    
    struct memblock_region *region = NULL;
    list_for_each_entry(region, &memblock.memory.regions, struct memblock_region, node) {
        map(kernel_mm_struct->pgdir, KERNEL_VA(region->base), region->base, region->size, VMA_R|VMA_W|VMA_X);
    }
    pgtbl_switch_to(kernel_mm_struct->pgdir);
    pgtbl_flush();
    current_mm_struct = NULL;
}


int copyin(pgtable_t *pagetable, char *dst, uintptr_t src_va, size_t len) {
    size_t n = 0;

    uintptr_t src = KERNEL_VA(pgtbl_lookup(pagetable, src_va));
    if (src == 0) {
        return -1;
    }

    memcpy(dst + n, (char *)src, len);
    return n;
}

int copyout(pgtable_t *pagetable, uintptr_t dst_va, char *src, size_t len) {
    size_t n = 0;
    uintptr_t dst = KERNEL_VA(pgtbl_lookup(pagetable, dst_va));
    if (dst == 0) {
        return -1;
    }
    memcpy((char *)dst, src + n, len);
    return n;
}