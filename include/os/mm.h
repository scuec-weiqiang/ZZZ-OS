/**
 * @FilePath: /ZZZ-OS/include/os/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 20:43:47
 * @LastEditTime: 2025-12-05 16:28:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <os/types.h>
#include <os/mm/pgtbl_types.h>
#include <os/mm/vma_flags.h>
#include <os/mm/mm_types.h>


extern pgtable_t *kernel_pgtbl; // 内核页全局目录

// extern void mm_init();
extern int map_range(pgtable_t *pgtbl, virt_addr_t vaddr, phys_addr_t paddr, size_t size, vma_flags_t flags);
extern int unmap_range(pgtable_t *pgtbl, virt_addr_t vaddr, size_t size);

#define LAZY_MAP 1
#define EAGER_MAP 0

extern int do_map(struct mm_struct *mm, virt_addr_t vaddr, phys_addr_t paddr, size_t size, vma_flags_t flags, int lazy_or_eager);
extern int do_unmap(struct mm_struct *mm, virt_addr_t vaddr, size_t size);

extern void kernel_page_table_init();
extern void build_kernel_mapping(struct mm_struct *mm);

#define ioremap(pa, size)   map_range(kernel_pgtbl, (uintptr_t)(pa), (uintptr_t)(pa), (size_t)(size), VMA_R | VMA_W )
#define iounmap(va)         unmap_range(kernel_pgtbl, (uintptr_t)(va))

#endif