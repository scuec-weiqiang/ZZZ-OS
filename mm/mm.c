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
        // if ((vaddr % page_size == 0) && (paddr % page_size == 0) && (size >= page_size)) {
        //     level = i;
        //     break;
        // }

        if ( (mod_u32(vaddr ,page_size) == 0) 
            && (mod_u32(paddr , page_size) == 0) && (size >= page_size)) {
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
    mm->pgdir = new_pgtbl();
    if (!mm->pgdir) {
        kfree(mm);
        return NULL;
    }
    mm->vma_list = (struct vma){0, 0, 0, mm, LIST_HEAD_INIT(mm->vma_list.node)}; // 初始化哨兵节点  
    mm->vma_count = 0;
    return mm;
}

void mm_destroy(struct mm_struct *mm) {
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

    /*
     * Temporary bring-up mode:
     * copy the whole root page table so we can rule out missing inherited
     * mappings while debugging user-mode page-table switches.
     */
    pgtbl_copy(dest_mm->pgdir, init_mm.pgdir, 0, 0, root_entries);

    /*
     * Original Linux-like behavior: only inherit the kernel half.
     *
     * CHECK(kernel_start_index >= 0 && kernel_start_index < root_entries,
     *       "copy_kernel_mapping: invalid kernel start index", return;);
     * pgtbl_copy(dest_mm->pgdir, init_mm.pgdir, 0,
     *            kernel_start_index, root_entries - kernel_start_index);
     */
}



void initial_mm_init() {
    extern char _early_pgtbl_start[];
    pgtable_t old_pgdir = {
        .root = (void *)(&_early_pgtbl_start),
        .root_pa = KERNEL_PA(&_early_pgtbl_start),
    };
    extern void arch_pgtbl_init(pgtable_t *tbl);
    arch_pgtbl_init(&old_pgdir);
    map(&old_pgdir, 0xffff0000,KERNEL_PA(trap_start), trap_size, PAGE_KERNEL_EXEC);

    pgtbl_switch_to(&old_pgdir);
    
    init_mm.pgdir = new_pgtbl();
    if (!init_mm.pgdir) {
        panic("failed to create kernel pgtable");
    }
    struct memblock_region *region = NULL;
    list_for_each_entry(region, &memblock.memory.region_head.node, struct memblock_region, node) {
        map(init_mm.pgdir, KERNEL_VA(region->base), region->base, region->size, PAGE_KERNEL|PROT_EXEC);
    }
    map(init_mm.pgdir, 0xffff0000,KERNEL_PA(trap_start), trap_size, PAGE_KERNEL_EXEC);
    map(init_mm.pgdir, 0x02020000,0x02020000, PAGE_SIZE, PAGE_DEVICE);

    // remap(init_mm.pgdir, trap_start, trap_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, text_start, text_size, PAGE_KERNEL_EXEC);

    // remap(init_mm.pgdir, data_start, data_size, PAGE_KERNEL);
    // remap(init_mm.pgdir, rodata_start, rodata_size, PAGE_KERNEL_RO);
    // remap(init_mm.pgdir, bss_start, bss_size, PAGE_KERNEL);
    // remap(init_mm.pgdir, initcall_start, initcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, exitcall_start, exitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, irqinitcall_start, irqinitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, irqexitcall_start, irqexitcall_size, PAGE_KERNEL_EXEC);
    // remap(init_mm.pgdir, early_stack_start, early_stack_size, PAGE_KERNEL);
    
    pgtbl_switch_to(init_mm.pgdir);
    
    
}

