/**
 * @FilePath: /ZZZ-OS/include/os/vm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:45
 * @LastEditTime: 2025-10-29 21:49:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _VM_H
#define _VM_H

#include "asm/mm.h"
#include "os/types.h"

enum pgt_size
{
    PAGE_SIZE_4K = 1UL << 12,
    PAGE_SIZE_2M = 1UL << 21,
    PAGE_SIZE_1G = 1UL << 30,
};

extern pgtbl_t* kernel_pgd;//kernel_page_global_directory 内核页全局目录

extern uintptr_t map_walk(pgtbl_t *pgd, uintptr_t va);
extern int mmap(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, enum pgt_size page_size, uint64_t flags);
extern int map_range(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags);
extern int copyin(pgtbl_t *pagetable, char *dst, uintptr_t src_va, size_t len);
extern int copyout(pgtbl_t *pagetable, uintptr_t dst_va, char *src, size_t len);
extern void kernel_page_table_init();
extern void page_table_init(pgtbl_t *pgd);

#endif