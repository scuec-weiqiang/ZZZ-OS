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

extern struct mm_struct *kernel_mm_struct;
extern struct mm_struct *current_mm_struct;

void mm_init();
struct mm_struct *mm_create(char *name);

#define LAZY_MAP 1
#define EAGER_MAP 0

int do_map(struct mm_struct *mm, virt_addr_t vaddr, phys_addr_t paddr, size_t size, vma_flags_t flags, int lazy_or_eager);
int do_unmap(struct mm_struct *mm, virt_addr_t vaddr, size_t size);

void build_kernel_mapping(struct mm_struct *mm);
void copy_kernel_mapping(struct mm_struct *dest_mm);

#define mmap(va, size, flags)   do_map(current_mm_struct, (va), 0, (size), (flags), LAZY_MAP)
#define ioremap(pa, size)   do_map(kernel_mm_struct, (uintptr_t)(pa), (uintptr_t)(pa), (size), VMA_R | VMA_W, EAGER_MAP)
#define iounmap(va)         do_unmap(kernel_mm_struct, (uintptr_t)(va), PAGE_SIZE)

int copyin(pgtable_t *pagetable, char *dst, uintptr_t src_va, size_t len);
int copyout(pgtable_t *pagetable, uintptr_t dst_va, char *src, size_t len);

#endif