/**
 * @FilePath: /ZZZ-OS/mm/mm.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-12-05 18:26:29
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <mm/mm_types.h>
#include <os/pfn.h>
#include <mm/pgtbl_types.h>
#include <mm/symbols.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <mm/memblock.h>
#include <mm/page.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/kva.h>
#include <mm/pgtbl.h>
#include <mm/vma.h>
#include <mm/pgprot.h>
#include <os/mm.h>
#include <mm/symbols.h>
#include <os/utils.h>
#include <os/err.h>

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

        if ((vaddr % page_size) == 0 &&
            (paddr % page_size) == 0 &&
            size >= page_size) {
            level = i;
            break;
        }
    }
    return level;
}

int map(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size, pgprot_t flags) {
    CHECK(pgtbl != NULL || size ==0, "pgtbl is NULL or size = 0", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);
    uintptr_t va = ALIGN_DOWN(vaddr, PAGE_SIZE);
    uintptr_t pa = ALIGN_DOWN(paddr, PAGE_SIZE);
    uintptr_t end = va + size;

    
    while (va < end) {
        int target_level = highest_possible_level(pgtbl, va, pa, end - va);
        int map_size = pgtbl_level_page_size(pgtbl, target_level);
        // printk("map: va=%xu to pa=%xu with flags %xu at level %d, map_size=%xu\n", va, pa, flags, target_level, map_size);
        int ret = pgtbl_map(pgtbl, va, pa, target_level, flags);
        if (ret < 0) {
            printk(RED("error: failed to map va=%xu to pa=%xu at level %d\n"), va, pa, target_level);
            return ret;
        }

        va += map_size;
        pa += map_size;
    }

    pgtbl_flush();
    return 0;
}

int remap(pgtable_t *pgtbl, virt_addr_t vaddr, size_t size, pgprot_t flags) {
    phys_addr_t paddr = pgtbl_lookup(pgtbl, vaddr);
    if (paddr == 0) {
        printk("remap failed: va=%xu is not mapped\n", vaddr);
        return -1;
    }
    size = ALIGN_UP(size, PAGE_SIZE);
    uintptr_t va = ALIGN_DOWN(vaddr, PAGE_SIZE);
    uintptr_t pa = ALIGN_DOWN(paddr, PAGE_SIZE);
    uintptr_t end = va + size;

    while (va < end) {
        int target_level = highest_possible_level(pgtbl, va, pa, end - va);
        int map_size = pgtbl_level_page_size(pgtbl, target_level);
    
        if (pgtbl_remap(pgtbl, va, pa, target_level, flags) < 0) {
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
    .refcount = 1,
    .pgdir = NULL,
    .vma_list = {0, 0, 0, &init_mm, {NULL, NULL}},
    .vma_count = 0,
};

struct mm_struct *mm_alloc() {
    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));
    if (!mm) {
        return NULL;
    }
    memset(mm, 0, sizeof(*mm));
    mm->refcount = 1;
    mm->pgdir = new_pgtbl();
    if (!mm->pgdir) {
        kfree(mm);
        return NULL;
    }
    mm->vma_list = (struct vma){0, 0, 0, mm, LIST_HEAD_INIT(mm->vma_list.node)}; // 初始化哨兵节点  
    mm->vma_count = 0;
    return mm;
}

struct mm_struct *mmget(struct mm_struct *mm)
{
    if (mm != NULL && mm != &init_mm) {
        __sync_add_and_fetch(&mm->refcount, 1);
    }
    return mm;
}

static void mm_free(struct mm_struct *mm) {
    struct list_head *pos, *n;

    if (mm == NULL) {
        return;
    }

    if (mm == &init_mm) {
        return;
    }

    list_for_each_safe(pos, n, &mm->vma_list.node) {
        struct vma *vma = list_entry(pos, struct vma, node);
        list_del(&vma->node);
        vma_destroy(vma);
    }

    if (mm->pgdir != NULL) {
        pgtbl_destroy(mm->pgdir);
        mm->pgdir = NULL;
    }

    kfree(mm);
}

void mmput(struct mm_struct *mm)
{
    int refcount;

    if (mm == NULL || mm == &init_mm) {
        return;
    }

    refcount = __sync_sub_and_fetch(&mm->refcount, 1);
    if (refcount < 0) {
        panic("mmput: refcount underflow mm=%xu\n", mm);
    }
    if (refcount == 0) {
        mm_free(mm);
    }
}

void mm_destroy(struct mm_struct *mm)
{
    mmput(mm);
}

int do_mmap(struct mm_struct * mm, virt_addr_t vaddr, size_t size, pgprot_t flags) {
    CHECK(mm != NULL, "mm is NULL", return -1;);

    size = ALIGN_UP(size, PAGE_SIZE);
    uintptr_t va = ALIGN_DOWN(vaddr, PAGE_SIZE);
    vma_add(mm, va, size, flags);

    pgtbl_flush();
    return 0;
}

int do_unmap(struct mm_struct *mm, virt_addr_t va, size_t size) {
    CHECK(mm != NULL, "mm is NULL", return -1;);
    CHECK(mm->pgdir != NULL, "mm pgdir is NULL", return -1;);

    size = ALIGN_UP(size, PAGE_SIZE);
    va = ALIGN_DOWN(va, PAGE_SIZE);

    CHECK(vma_delete(mm, va, size) == 0, "vma delete failed", return -1;);
    return unmap(mm->pgdir, va, size);
}

static virt_addr_t alloc_mmio_va(size_t size) {
    static virt_addr_t current_mmio_va = KERNEL_MMIO_BASE;
    for (int i = 0; i < init_mm.pgdir->features->support_levels; i++) {
        size_t page_size = init_mm.pgdir->features->level[i].page_size;
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
    // dprintk("ioremap: pa=%xu size=%xu to va=%xu\n", pa, size, va);
    map(init_mm.pgdir, va, pa, size, PAGE_DEVICE);
    return (void *)va;
}

void iounmap(virt_addr_t va, size_t size) {
    unmap(init_mm.pgdir, va, size);
}

void copy_kernel_mapping(struct mm_struct *dest_mm) {
    int root_entries;
    int kernel_start_index;

    CHECK(dest_mm != NULL, "copy_kernel_mapping: dest_mm is NULL", return;);
    CHECK(dest_mm->pgdir != NULL, "copy_kernel_mapping: dest pgdir is NULL", return;);
    CHECK(init_mm.pgdir != NULL, "copy_kernel_mapping: init pgdir is NULL", return;);

    root_entries = init_mm.pgdir->features->level[0].table_size / sizeof(pte_t);
    kernel_start_index = pgtbl_level_index(init_mm.pgdir, 0, KERNEL_VA_BASE);

    CHECK(kernel_start_index >= 0 && kernel_start_index < root_entries,
          "copy_kernel_mapping: invalid kernel start index", return;);
    pgtbl_copy(dest_mm->pgdir, init_mm.pgdir, 0,
               kernel_start_index, root_entries - kernel_start_index);
}



void initial_mm_init() {
    arch_initial_mm_init(); 
}
