/**
 * @FilePath: /ZZZ/kernel/vm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:45
 * @LastEditTime: 2025-09-21 00:07:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _VM_H
#define _VM_H

#include "mm.h"
#include "types.h"

extern pgtbl_t* kernel_pgd;//kernel_page_global_directory 内核页全局目录

extern pgtbl_t* get_child_pgtbl(pgtbl_t *parent_pgd, u64 vpn_level, u64 va, bool create);
extern pte_t* page_walk(pgtbl_t *pgd, uintptr_t va, bool create);
extern int map_pages(pgtbl_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, u64 flags);
extern void kernel_page_table_init();
extern void page_table_init(pgtbl_t *pgd);

#endif