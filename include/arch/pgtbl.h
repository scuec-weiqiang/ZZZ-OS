/**
 * @FilePath: /ZZZ-OS/include/arch/pgtbl.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-12-04 19:04:15
 * @LastEditTime: 2025-12-05 16:58:10
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __ARCH_PGTBL_H
#define __ARCH_PGTBL_H

#include <os/types.h>
#include <os/pgtbl_types.h>

extern pteval_t pa_to_pteval(phys_addr_t pa);
extern phys_addr_t pteval_to_pa(pteval_t val);
extern void set_pte(pte_t* pte, phys_addr_t pa, uint32_t flags);
extern int is_pte_leaf(pte_t *pte);
extern int arch_pgtbl_init(pgtbl_t *pgtbl);
extern int arch_map(pgtbl_t *pgd, virt_addr_t va, phys_addr_t pa, enum huge_page huge_page, uint32_t flags);
extern int arch_unmap(pgtbl_t *pgd, virt_addr_t va);
extern void arch_pgtbl_flush();
extern phys_addr_t arch_pgtbl_walk(pgtbl_t *pgd, virt_addr_t va);
extern void arch_pgtbl_switch(pgtbl_t *pgd);

// extern void arch_pgtbl_test();
#endif // __ARCH_PGTBL_H