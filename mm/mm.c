/**
 * @FilePath: /ZZZ-OS/mm/mm.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-11-24 22:41:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <arch/pgtbl.h>
#include <os/mm/symbols.h>
#include <drivers/virtio.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/mm/page.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/mm.h>
#include <os/mm/early_malloc.h>

pgtbl_t *kernel_pgtbl = NULL; // kernel_page_global_directory 内核页全局目录

pgtbl_t *new_pgtbl() {
    pgtbl_t *tbl = kmalloc(sizeof(pgtbl_t));
    if (!tbl) {
        return NULL;
    }

    memset(tbl, 0, sizeof(pgtbl_t));

    if (arch_pgtbl_init(tbl) < 0) {
        kfree(tbl);
        return NULL;
    }

    return tbl;
}

int map_range(pgtbl_t *pgtbl, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(vaddr % BIG_PAGE_4K == 0, "vaddr is not page aligned", return -1;);
    CHECK(paddr % BIG_PAGE_4K == 0, "paddr is not page aligned", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);

    uintptr_t va = vaddr;
    uintptr_t pa = paddr;
    uintptr_t end = vaddr + size;

    while (va < end) {
        enum big_page chunk_size;

        // 能否用 1GB 大页
        if ((va % BIG_PAGE_1G == 0) && (pa % BIG_PAGE_1G == 0) && (end - va) >= BIG_PAGE_1G) {
            chunk_size = BIG_PAGE_1G;
        }
        // 能否用 2MB 大页
        else if ((va % BIG_PAGE_2M == 0) && (pa % BIG_PAGE_2M == 0) && (end - va) >= BIG_PAGE_2M) {
            chunk_size = BIG_PAGE_2M;
        }
        // 否则用 4KB
        else {
            chunk_size = BIG_PAGE_4K;
        }

        if (arch_map(pgtbl, va, pa, chunk_size, flags) < 0) {
            return -1;
        }

        va += chunk_size;
        pa += chunk_size;
    }

    arch_pgtbl_flush();
    return 0;
}

int unmap_range(pgtbl_t *pgtbl, uintptr_t va) {
    
    return arch_unmap(pgtbl, va);
}

void pgtbl_switch(pgtbl_t *pgtbl) {
    arch_pgtbl_switch(pgtbl);
}

void pgtbl_flush() {
    arch_pgtbl_flush();
}

void build_kernel_mapping(pgtbl_t *pgtbl) {
    // 映射内核代码段，数据段，栈以及堆的保留页到虚拟地址空间
    map_range(pgtbl, (uintptr_t)text_start, (uintptr_t)KERNEL_PA(text_start), (size_t)text_size, PTE_R | PTE_X);
    map_range(pgtbl, (uintptr_t)rodata_start, (uintptr_t)KERNEL_PA(rodata_start), (size_t)rodata_size, PTE_R);
    map_range(pgtbl, (uintptr_t)data_start, (uintptr_t)KERNEL_PA(data_start), (size_t)data_size, PTE_R | PTE_W);
    map_range(pgtbl, (uintptr_t)bss_start, (uintptr_t)KERNEL_PA(bss_start), (size_t)bss_size, PTE_R | PTE_W);
    
    map_range(pgtbl, (uintptr_t)initcall_start, (uintptr_t)KERNEL_PA(initcall_start), (size_t)initcall_size, PTE_R | PTE_W | PTE_X);
    map_range(pgtbl, (uintptr_t)exitcall_start, (uintptr_t)KERNEL_PA(exitcall_start), (size_t)exitcall_size , PTE_R | PTE_W | PTE_X);
    map_range(pgtbl, (uintptr_t)irqinitcall_start, (uintptr_t)KERNEL_PA(irqinitcall_start), (size_t)irqinitcall_size, PTE_R | PTE_W | PTE_X);
    map_range(pgtbl, (uintptr_t)irqexitcall_start, (uintptr_t)KERNEL_PA(irqexitcall_start), (size_t)irqexitcall_size , PTE_R | PTE_W | PTE_X);
    map_range(pgtbl, (uintptr_t)early_stack_start, (uintptr_t)KERNEL_PA(early_stack_start), (size_t)early_stack_size, PTE_R | PTE_W);

    // map_range(pgtbl, kernel_start, KERNEL_PA(kernel_start), kernel_size, PTE_R | PTE_W | PTE_X);
    map_range(pgtbl, (uintptr_t)heap_start, (uintptr_t)KERNEL_PA(heap_start), (size_t)heap_size, PTE_R | PTE_W);
    map_range(pgtbl, (uintptr_t)stack_start, (uintptr_t)KERNEL_PA(stack_start), (size_t)stack_size * 2, PTE_R | PTE_W);

    map_range(pgtbl, KERNEL_VA(early_malloc_start), early_stack_start, early_stack_size, PTE_R | PTE_W );
    map_range(pgtbl, (uintptr_t)VIRTIO_MMIO_BASE, (uintptr_t)VIRTIO_MMIO_BASE, PAGE_SIZE, PTE_R | PTE_W);
    map_range(pgtbl, (uintptr_t)0x10000000, (uintptr_t)0x10000000, PAGE_SIZE, PTE_R | PTE_W);

}

void kernel_page_table_init() {
    kernel_pgtbl = mm_new_pgtbl();
    if (kernel_pgtbl == NULL)
        return;
    build_kernel_mapping(kernel_pgtbl);
    
    pgtbl_switch(kernel_pgd);
    pgtbl_flush();
    printk("kernel page init success!\n");
}

int copyin(pgtbl_t *pagetable, char *dst, uintptr_t src_va, size_t len) {
    size_t n = 0;
    while (n < len) {
        uintptr_t src = arch_walk(pagetable, src_va);
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

int copyout(pgtbl_t *pagetable, uintptr_t dst_va, char *src, size_t len) {
    size_t n = 0;
    while (n < len) {
        uintptr_t dst = arch_va_to_pa(pagetable, dst_va);
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