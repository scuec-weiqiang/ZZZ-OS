/**
 * @FilePath: /ZZZ-OS/include/arch/pgtbl.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-12-04 19:04:15
 * @LastEditTime: 2025-12-04 23:28:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __ARCH_PGTBL_H
#define __ARCH_PGTBL_H

#include <os/types.h>
#include <os/pgtbl_types.h>

extern int arch_pgtbl_init(pgtbl_t *pgtbl);
extern int arch_map(pgtbl_t *pgd, virt_addr_t va, phys_addr_t pa, enum big_page big_page, uint32_t flags);
extern int arch_unmap(pgtbl_t *pgd, virt_addr_t va);
extern void arch_pgtbl_flush();
extern void* arch_pgtbl_walk(pgtbl_t *pgd, virt_addr_t va);
extern void arch_pgtbl_switch(pgtbl_t *pgd);

#endif // __ARCH_PGTBL_H