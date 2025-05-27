/**
 * @FilePath: /ZZZ/kernel/vm.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:50
 * @LastEditTime: 2025-05-26 02:32:08
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printf.h"
#include "virtio.h"
#include "clint.h"
#include "plic.h"
#include "uart.h"
#include "page_alloc.h"

#include "riscv.h"
#include "mm.h"
#include "platform.h"
#include "string.h"
#include "maddr_def.h"

pgtbl_t* kernel_pgd = NULL;//kernel_page_global_directory 内核页全局目录

/**
 * @brief 从父页表中获取子页表
 *
 * 根据虚拟页号(vpn_level)和虚拟地址(va)从父页表(parent_pgd)中获取对应的子页表。
 * 如果子页表不存在且参数create为true，则创建子页表。
 *
 * @param parent_pgd 父页表的地址
 * @param vpn_level 虚拟页号
 * @param va 虚拟地址
 * @param create 是否创建子页表，true表示创建，false表示不创建
 *
 * @return 指向子页表的指针，如果不存在且不创建则返回NULL
 */
pgtbl_t* get_child_pgtbl(pgtbl_t *parent_pgd, uint64_t vpn_level, uint64_t va, bool create)
{
    if(parent_pgd == NULL) return NULL;
    
    pgtbl_t* child_pgd = NULL;
    if( (parent_pgd[vpn_level] & PTE_V) == 0)//验证对应子页表是否存在，
    {
        if(!create) 
        {
            return NULL;//如果不存在,但指明不需要创建就返回
        }
        else //否则创建对应子页表
        {
            child_pgd = (pgtbl_t*)page_alloc(1);
            if(child_pgd == NULL) return NULL;
            memset(child_pgd,0,PAGE_SIZE);
            //设置对应子页表的物理地址，并标记为有效（PTE_V
            parent_pgd[vpn_level] = PA2PTE(child_pgd) | PTE_V;
            return child_pgd;
        }
    }
    else //如果存在，直接返回对应pmd的物理地址
    {
       //返回对应pmd的物理地址
        return (pgtbl_t*)PTE2PA(parent_pgd[vpn_level]);
    }
}

/**
 * @brief 页表遍历函数
 *
 * 该函数根据给定的页全局目录指针（pgd）、虚拟地址（va）和是否创建页表项的布尔值（create），遍历页表并返回对应的页表项指针（pte_t*）。
 *
 * @param pgd 页全局目录指针
 * @param va 虚拟地址
 * @param create 是否创建页表项
 *
 * @return 对应的页表项指针（pte_t*），如果未找到对应的页表项，则返回NULL。
 */
pte_t* page_walk(pgtbl_t *pgd, uint64_t va, bool create)
{
    if(pgd == NULL) return NULL;
    if(va % PAGE_SIZE != 0) return NULL;

    uint64_t *pmd = NULL;
    uint64_t *pte = NULL;

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    pmd = get_child_pgtbl(pgd,vpn2,va,true);//获取对应pmd的物理地址
    if(pmd == NULL) return NULL;
    
    pte = get_child_pgtbl(pmd,vpn1,va,true);
    if (pte == NULL) return NULL;
    
    return (pte_t*)&pte[vpn0];
}

/**
 * @brief 将物理地址映射到虚拟地址空间
 *
 * 该函数将指定的物理地址范围内的内存映射到虚拟地址空间。
 *
 * @param pgd 页表目录指针
 * @param vaddr 起始虚拟地址
 * @param paddr 起始物理地址
 * @param size 需要映射的内存大小
 * @param flags 页表项标志位
 *
 * @return 成功时返回0，失败时返回-1
 */
int map_pages(pgtbl_t *pgd, uint64_t vaddr, uint64_t paddr, size_t size, uint64_t flags)
{
    // 检查 pgd 是否为空
    if(pgd == NULL) return -1;
    // 检查 size 是否为0
    if(size == 0) return -1;
    // 检查虚拟地址和物理地址是否对齐到页面大小
    if(vaddr % PAGE_SIZE != 0 || paddr % PAGE_SIZE != 0) return -1;
    // 检查 size 是否为页面大小的整数倍
    if(size % PAGE_SIZE != 0) return -1;
    // 遍历所有需要映射的页，并设置对应的页表项（PTE）
    for(uint64_t va = vaddr; va <= vaddr+size; va += PAGE_SIZE, paddr += PAGE_SIZE) 
    {
        // 在页表中查找或创建页表项（PTE）
        pte_t *pte = page_walk(pgd, va, true);
        if (pte == NULL)
        {
            // 无法找到或创建页表项，返回错误码
            return -1;
        }
        // 设置页表项（PTE）的值
        *pte = PA2PTE(paddr) | flags | PTE_V;
        // printf("va = %x,pa = %x,pte = %x,pte_value = %x\n",va,paddr,*pte,((paddr>>12)<<10));
    }
}


void kernel_page_table_init()
{
    kernel_pgd = (pgtbl_t*)page_alloc(1);
    if(kernel_pgd == NULL) return;
    memset(kernel_pgd,0,PAGE_SIZE);

    // map_pages(kernel_pgd,0x80000000,0x80000000,0x8000000,PTE_R | PTE_W |PTE_X);
    // 映射内核代码段，数据段，栈以及堆的保留页到虚拟地址空间 
    map_pages(kernel_pgd,_text_start,_text_start,_text_size,PTE_R | PTE_X);
    map_pages(kernel_pgd,_rodata_start,_rodata_start,_rodata_size,PTE_R);
    map_pages(kernel_pgd,_data_start,_data_start,_data_size,PTE_R | PTE_W);
    map_pages(kernel_pgd,_bss_start,_bss_start,_bss_size,PTE_R | PTE_W);
    map_pages(kernel_pgd,_stack_start,_stack_start,_stack_size,PTE_R | PTE_W);
    map_pages(kernel_pgd,_heap_start,_heap_start,RESERVED_PAGE_SIZE,PTE_R | PTE_W);

    //映射外设寄存器地址空间到内核虚拟地址空间
    map_pages(kernel_pgd,CLINT_BASE,CLINT_BASE,PAGE_SIZE,PTE_R | PTE_W);
    map_pages(kernel_pgd,PLIC_BASE,PLIC_BASE,0x200*PAGE_SIZE,PTE_R | PTE_W);
    map_pages(kernel_pgd,UART_BASE,UART_BASE,PAGE_SIZE,PTE_R | PTE_W);
    map_pages(kernel_pgd,VIRTIO_MMIO_BASE,VIRTIO_MMIO_BASE,PAGE_SIZE,PTE_R | PTE_W);
    //设置satp寄存器
    satp_w(MAKE_SATP(kernel_pgd));
    printf("kernel page table init success!\n");
}