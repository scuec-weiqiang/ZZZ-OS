/**
 * @FilePath: /ZZZ-OS/include/os/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 20:43:47
 * @LastEditTime: 2025-11-11 23:41:35
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <os/types.h>

typedef struct pgtbl pgtbl_t;

#define PTE_V (1 << 0)      // 有效位
#define PTE_R (1 << 1)      // 可读
#define PTE_W (1 << 2)      // 可写
#define PTE_X (1 << 3)      // 可执行
#define PTE_U (1 << 4)      // 用户模式可访问

#define KERNEL_PA_BASE 0x80000000
#define KERNEL_VA_BASE 0xffffffffc0000000
#define KERNEL_VA_START 0xffffffffc0200000
#define KERNEL_VA(pa) (KERNEL_VA_BASE + ((uint64_t)(pa)) - KERNEL_PA_BASE)
#define KERNEL_PA(va) ((uint64_t)(va) - KERNEL_VA_BASE + KERNEL_PA_BASE)

/**
 * 用户虚拟地址空间描述
 */
struct mm_struct {
    pgtbl_t *pgtbl;  // 页表指针
    uintptr_t start_brk;
    uintptr_t brk;
    uintptr_t start_stack;
    uintptr_t mmap_base;     // mmap 区域基地址
    // struct vm_area_struct *vma_list;
};

extern pgtbl_t *kernel_pgd; // 内核页全局目录

extern void mm_init();
extern int map_range(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags);
extern int unmap_range(pgtbl_t *pgd, uintptr_t vaddr);
extern pgtbl_t* mm_new_pgtbl();
extern void mm_switch_pgtbl(pgtbl_t *pgd);
extern void mm_flush_pgtbl();
extern uintptr_t va_to_pa(pgtbl_t *pgd, uintptr_t va);

extern void kernel_page_table_init();
extern void page_table_init(pgtbl_t *pgd);

#define ioremap(pa, size)   map_range(kernel_pgd, (uintptr_t)(pa), (uintptr_t)(pa), (size_t)(size), PTE_R | PTE_W )
#define iounmap(va)         unmap_range(kernel_pgd, (uintptr_t)(va))

#endif