/**
 * @FilePath: /ZZZ-OS/mm/mm.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-11-24 22:41:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <asm/mm.h>
#include <asm/pgtbl.h>
#include <asm/riscv.h>
#include <os/mm/symbols.h>
#include <drivers/virtio.h>
#include <os/check.h>
#include <os/malloc.h>
#include <os/page.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/mm.h>

pgtbl_t *kernel_pgd = NULL; // kernel_page_global_directory 内核页全局目录

pgtbl_t *mm_new_pgtbl() {
    return arch_new_pgtbl();
}

int map_range(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags) {
    CHECK(pgd != NULL, "pgd is NULL", return -1;);
    CHECK(vaddr % PAGE_SIZE_4K == 0, "vaddr is not page aligned", return -1;);
    CHECK(paddr % PAGE_SIZE_4K == 0, "paddr is not page aligned", return -1;);
    size = ALIGN_UP(size, PAGE_SIZE);

    uintptr_t va = vaddr;
    uintptr_t pa = paddr;
    uintptr_t end = vaddr + size;

    while (va < end) {
        enum page_size chunk_size;

        // 能否用 1GB 大页
        if ((va % PAGE_SIZE_1G == 0) && (pa % PAGE_SIZE_1G == 0) && (end - va) >= PAGE_SIZE_1G) {
            chunk_size = PAGE_SIZE_1G;
        }
        // 能否用 2MB 大页
        else if ((va % PAGE_SIZE_2M == 0) && (pa % PAGE_SIZE_2M == 0) && (end - va) >= PAGE_SIZE_2M) {
            chunk_size = PAGE_SIZE_2M;
        }
        // 否则用 4KB
        else {
            chunk_size = PAGE_SIZE_4K;
        }

        if (arch_map(pgd, va, pa, chunk_size, flags) < 0) {
            return -1;
        }

        va += chunk_size;
        pa += chunk_size;
    }

    arch_flush_pgtbl();
    return 0;
}

int unmap_range(pgtbl_t *pgd, uintptr_t va) {
    
    return arch_unmap(pgd, va);
}

void mm_switch_pgtbl(pgtbl_t *pgd) {
    arch_switch_pgtbl(pgd);
}

void mm_flush_pgtbl() {
    arch_flush_pgtbl();
}

void page_table_init(pgtbl_t *pgd) {
    // 映射内核代码段，数据段，栈以及堆的保留页到虚拟地址空间
    // map_range(pgd, (uintptr_t)text_start, (uintptr_t)KERNEL_PA(text_start), (size_t)text_size, PTE_R | PTE_X);
    // map_range(pgd, (uintptr_t)rodata_start, (uintptr_t)KERNEL_PA(rodata_start), (size_t)rodata_size, PTE_R);
    // map_range(pgd, (uintptr_t)data_start, (uintptr_t)KERNEL_PA(data_start), (size_t)data_size, PTE_R | PTE_W);
    // map_range(pgd, (uintptr_t)bss_start, (uintptr_t)KERNEL_PA(bss_start), (size_t)bss_size, PTE_R | PTE_W);
    
    // map_range(pgd, (uintptr_t)initcall_start, (uintptr_t)KERNEL_PA(initcall_start), (size_t)initcall_size, PTE_R | PTE_W | PTE_X);
    // map_range(pgd, (uintptr_t)exitcall_start, (uintptr_t)KERNEL_PA(exitcall_start), (size_t)exitcall_size , PTE_R | PTE_W | PTE_X);
    // map_range(pgd, (uintptr_t)irqinitcall_start, (uintptr_t)KERNEL_PA(irqinitcall_start), (size_t)irqinitcall_size, PTE_R | PTE_W | PTE_X);
    // map_range(pgd, (uintptr_t)irqexitcall_start, (uintptr_t)KERNEL_PA(irqexitcall_start), (size_t)irqexitcall_size , PTE_R | PTE_W | PTE_X);
    // map_range(pgd, (uintptr_t)early_stack_start, (uintptr_t)KERNEL_PA(early_stack_start), (size_t)early_stack_size, PTE_R | PTE_W);

    map_range(pgd, kernel_start, KERNEL_PA(kernel_start), kernel_size, PTE_R | PTE_W | PTE_X);
    map_range(pgd, (uintptr_t)heap_start, (uintptr_t)KERNEL_PA(heap_start), (size_t)heap_size, PTE_R | PTE_W);
    map_range(pgd, (uintptr_t)stack_start, (uintptr_t)KERNEL_PA(stack_start), (size_t)stack_size * 2, PTE_R | PTE_W);

    map_range(pgd, (uintptr_t)VIRTIO_MMIO_BASE, (uintptr_t)VIRTIO_MMIO_BASE, PAGE_SIZE, PTE_R | PTE_W);
    map_range(pgd, (uintptr_t)0x10000000, (uintptr_t)0x10000000, PAGE_SIZE, PTE_R | PTE_W);

}

void kernel_page_table_init() {
    kernel_pgd = (pgtbl_t *)arch_new_pgtbl();
    if (kernel_pgd == NULL)
        return;
    page_table_init(kernel_pgd);
    
    arch_switch_pgtbl(kernel_pgd);
    arch_flush_pgtbl();
    // virtio->queue_notify = 24; // 设置 virtio 的 queue_notify 地址
    printk("kernel page init success!\n");
    // printk("virtio = %p\n", virtio);
    
        uint64_t satp = satp_r();
printk("SATP raw = %xu\n", satp);
printk("SATP mode = %xu, asid = %xu, root_ppn = %xu\n",
       (satp >> 60) & 0xf,
       (satp >> 44) & 0xffff,
       satp & ((1ULL<<44)-1));
    uint64_t a = arch_va_to_pa(kernel_pgd, 0x80200000);
    printk("va 0x80200000 to pa = %xu\n", a);
    a = arch_va_to_pa(kernel_pgd, heap_start);
    printk("va heap_start to pa = %xu\n", a);
    a = arch_va_to_pa(kernel_pgd, stack_start);
    printk("va stack_start to pa = %xu\n", a);
    a = arch_va_to_pa(kernel_pgd, VIRTIO_MMIO_BASE);
    printk("va VIRTIO_MMIO_BASE to pa = %xu\n", a);

    a = arch_va_to_pa(kernel_pgd, 0x80349438);
    printk("va 0x80349438 to pa = %xu\n", a);
}

int copyin(pgtbl_t *pagetable, char *dst, uintptr_t src_va, size_t len) {
    size_t n = 0;
    while (n < len) {
        uintptr_t src = arch_va_to_pa(pagetable, src_va);
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