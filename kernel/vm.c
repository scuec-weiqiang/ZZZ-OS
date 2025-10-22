/**
 * @FilePath: /ZZZ-OS/kernel/vm.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-10-22 23:07:16
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printk.h"
#include "virtio.h"
#include "uart.h"
#include "malloc.h"
#include "riscv.h"
#include "mm.h"
#include "string.h"
#include "symbols.h"
#include "plic.h"
#include "clint.h"
#include "vm.h"
#include "check.h"


pgtbl_t* kernel_pgd = NULL;//kernel_page_global_directory 内核页全局目录

pgtbl_t* create_pgtbl()
{
    pgtbl_t *pgd = (pgtbl_t*)page_alloc(1);
    memset(pgd,0,PAGE_SIZE);
    return pgd;
}

void create_pte(pte_t *pte, uintptr_t pa, u64 flags)
{
    *pte = PA2PTE(pa) | flags | PTE_V;
}

pgtbl_t* get_child_pgtbl(pgtbl_t *parent_pgd, u64 vpn, bool create)
{
    if(parent_pgd == NULL) return NULL;

    pgtbl_t* child_pgd = NULL;
    if( (parent_pgd[vpn] & PTE_V) == 0)//如果不存在,返回
    {
        if(!create) 
        {
            return NULL;//如果不存在,但指明不需要创建就返回
        }
        else //否则创建对应子页表
        {
            child_pgd = create_pgtbl();
            if(child_pgd == NULL) return NULL;
            create_pte(&parent_pgd[vpn], KERNEL_PA(child_pgd), 0);
            return child_pgd;
        }
        return NULL;
    }
    else //如果存在
    {
        return (pgtbl_t*)KERNEL_VA(PTE2PA(parent_pgd[vpn]));
    }
}

int is_pte_leaf(pte_t pte)
{
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

uintptr_t map_walk(pgtbl_t *pgd, uintptr_t va)
{
    CHECK(pgd != NULL, "pgd is NULL", return 0;);

    u64 vpn2 = (va >> 30) & 0x1ff;
    u64 vpn1 = (va >> 21) & 0x1ff;
    u64 vpn0 = (va >> 12) & 0x1ff;

    va = ALIGN_DOWN(va, PAGE_SIZE);

    // enum pgt_size page_size;

    uintptr_t l2 = (uintptr_t)get_child_pgtbl(pgd, vpn2, false);
    if(is_pte_leaf(l2)) // 1GB 大页
    {
        return PTE2PA(l2);
    }

    uintptr_t l1 = (uintptr_t)get_child_pgtbl((pgtbl_t*)l2, vpn1, false);
    if(is_pte_leaf(l1)) // 2MB 大页
    {
        return PTE2PA(l1);
    }

    uintptr_t l0 = (uintptr_t)get_child_pgtbl((pgtbl_t*)l1, vpn0, false);
    return l0;
}

int mmap(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, enum pgt_size page_size, u64 flags)
{
    CHECK(pgd != NULL, "pgd is NULL", return -1;);
    CHECK(vaddr % page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(paddr % page_size == 0, "paddr is not page aligned", return -1;);

    u64 vpn2 = (vaddr >> 30) & 0x1ff;
    u64 vpn1 = (vaddr >> 21) & 0x1ff;
    u64 vpn0 = (vaddr >> 12) & 0x1ff;

    switch (page_size)
    {
        case PAGE_SIZE_4K:
        {
            pgtbl_t *l1 = get_child_pgtbl(pgd, vpn2, true);
            pgtbl_t *l0 = get_child_pgtbl(l1, vpn1, true);
            pte_t *pte = &l0[vpn0];
            create_pte(pte, paddr, flags);
            break;
        }
            
        case PAGE_SIZE_2M:
        {
            pgtbl_t *l1 = get_child_pgtbl(pgd, vpn2, true);
            pte_t *pte = &l1[vpn1];
            create_pte(pte, paddr, flags);
            break;
        }

        case PAGE_SIZE_1G:
        {
            pte_t *pte = &pgd[vpn2];
            create_pte(pte, paddr, flags);
            break;
        }

        default:
        {
            printk("Unsupported page size\n");
            return -1;
        }
    }

    return 0;
}

int map_range(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, u64 flags)
{
    CHECK(pgd != NULL, "pgd is NULL", return -1;);
    CHECK(vaddr % PAGE_SIZE_4K == 0, "vaddr is not page aligned", return -1;);
    CHECK(paddr % PAGE_SIZE_4K == 0, "paddr is not page aligned", return -1;);

    size = ALIGN_UP(size, PAGE_SIZE);
    
    uintptr_t va = vaddr;
    uintptr_t pa = paddr;
    uintptr_t end = vaddr + size;

    while (va < end) 
    {
        enum pgt_size chunk_size;

        // 能否用 1GB 大页
        if ((va % PAGE_SIZE_1G == 0) && (pa % PAGE_SIZE_1G == 0) && (end - va) >= PAGE_SIZE_1G) 
        {
            chunk_size = PAGE_SIZE_1G;
        }
        // 能否用 2MB 大页
        else if ((va % PAGE_SIZE_2M == 0) && (pa % PAGE_SIZE_2M == 0) && (end - va) >= PAGE_SIZE_2M) 
        {
            chunk_size = PAGE_SIZE_2M;
        }
        // 否则用 4KB
        else 
        {
            chunk_size = PAGE_SIZE_4K;
        }

        if (mmap(pgd, va, pa, chunk_size, flags) < 0) {
            return -1;
        }

        va += chunk_size;
        pa += chunk_size;
    }
    asm volatile("sfence.vma zero, zero"); // 刷新 TLB
    return 0;
}

void page_table_init(pgtbl_t *pgd)
{
        // 映射内核代码段，数据段，栈以及堆的保留页到虚拟地址空间 
        map_range(pgd,(uintptr_t)text_start,(uintptr_t)KERNEL_PA(text_start),(size_t)text_size,PTE_R|PTE_X);
        map_range(pgd,(uintptr_t)rodata_start,(uintptr_t)KERNEL_PA(rodata_start),(size_t)rodata_size,PTE_R);
        map_range(pgd,(uintptr_t)data_start,(uintptr_t)KERNEL_PA(data_start),(size_t)data_size,PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)bss_start ,(uintptr_t)KERNEL_PA(bss_start),(size_t)bss_size,PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)dtb_start ,(uintptr_t)KERNEL_PA(dtb_start),(size_t)dtb_size,PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)heap_start,(uintptr_t)KERNEL_PA(heap_start),(size_t)heap_size,PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)stack_start,(uintptr_t)KERNEL_PA(stack_start),(size_t)stack_size*2,PTE_R|PTE_W);

        //恒等映射外设寄存器地址空间到内核虚拟地址空间
        map_range(pgd,(uintptr_t)CLINT_BASE, (uintptr_t)CLINT_BASE, 12*PAGE_SIZE, PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)PLIC_BASE, (uintptr_t)PLIC_BASE, 0x200*PAGE_SIZE, PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)UART_BASE, (uintptr_t)UART_BASE, PAGE_SIZE, PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)VIRTIO_MMIO_BASE, (uintptr_t)VIRTIO_MMIO_BASE, PAGE_SIZE, PTE_R|PTE_W);
        map_range(pgd,(uintptr_t)REAL_TIME_BASE, (uintptr_t)REAL_TIME_BASE, PAGE_SIZE, PTE_R);
}

void kernel_page_table_init()  
{
    kernel_pgd = (pgtbl_t*)page_alloc(1);
    if(kernel_pgd == NULL) return;
    memset(kernel_pgd,0,PAGE_SIZE);

    page_table_init(kernel_pgd);

    uintptr_t satp_val = make_satp((uintptr_t)kernel_pgd);

    //设置satp寄存器
    asm volatile("csrw satp,%0"::"r"(satp_val));
    asm volatile("sfence.vma zero, zero");
    printk("kernel page table init success!\n");
}

int copyin(pgtbl_t *pagetable, char *dst, uintptr_t src_va, size_t len)
{
    size_t n = 0;
    while (n < len) 
    {
        uintptr_t src = map_walk(pagetable, src_va);
        if (src == 0) 
        {
            return -1;
        }
        size_t offset = src_va % PAGE_SIZE;
        size_t to_copy = PAGE_SIZE - offset;
        if (to_copy > len - n) 
        {
            to_copy = len - n;
        }

        memcpy(dst + n, (char *)(src + offset), to_copy);

        n += to_copy;
        src_va += to_copy;
    }
    return n;
}

int copyout(pgtbl_t *pagetable, uintptr_t dst_va, char *src, size_t len)
{
    size_t n = 0;
    while (n < len) 
    {
        uintptr_t dst = map_walk(pagetable, dst_va);
        if (dst == 0) 
        {
            return -1;
        }
        size_t offset = dst_va % PAGE_SIZE;
        size_t to_copy = PAGE_SIZE - offset;
        if (to_copy > len - n) 
        {
            to_copy = len - n;
        }

        memcpy((char *)(dst + offset), src + n, to_copy);

        n += to_copy;
        dst_va += to_copy;
    }
    return n;
}