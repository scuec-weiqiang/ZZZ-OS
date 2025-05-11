/**
 * @FilePath: /ZZZ/kernel/vm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-08 22:00:45
 * @LastEditTime: 2025-05-09 21:56:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _VM_H
#define _VM_H

#include "mm.h"

extern pgtbl_t* get_child_pgtbl(pgtbl_t *parent_pgd, uint64_t vpn_level, uint64_t va, bool create);
extern pte_t* page_walk(pgtbl_t *pgd, uint64_t va, bool create);
extern int map_pages(pgtbl_t *pgd, uint64_t vaddr, uint64_t paddr, size_t size, uint64_t flags);

extern void kernel_page_table_init();


#endif