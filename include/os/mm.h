/**
 * @FilePath: /ZZZ-OS/include/os/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 20:43:47
 * @LastEditTime: 2025-10-31 00:51:12
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <os/types.h>

typedef struct pgtbl pgtbl_t;

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
extern void mm_switch_pgtbl(pgtbl_t *pgd);
extern void mm_flush_pgtbl();
extern uintptr_t va_to_pa(pgtbl_t *pgd, uintptr_t va);

extern void kernel_page_table_init();
extern void page_table_init(pgtbl_t *pgd);

#endif