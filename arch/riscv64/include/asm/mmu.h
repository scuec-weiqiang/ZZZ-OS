/**
 * @FilePath: /ZZZ-OS/arch/riscv64/include/asm/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 20:43:47
 * @LastEditTime: 2025-12-04 13:45:07
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __ASM_MM_H__
#define __ASM_MM_H__

#include <os/types.h>

typedef struct pgtbl pgtbl_t;

// 通用页表操作接口（各架构实现）
extern void arch_mmu_init(void);
extern pgtbl_t *arch_new_pgtbl();
extern int arch_map(pgtbl_t *pgd, uintptr_t va, uintptr_t pa, enum big_page page_size, uint32_t flags);
extern int arch_unmap(pgtbl_t *pgd, uintptr_t va);
extern void arch_flush_pgtbl();
extern uintptr_t arch_va_to_pa(pgtbl_t *pgd, uintptr_t va);
extern void arch_switch_pgtbl(pgtbl_t *pgd);

#endif // __ASM_MM_H__
