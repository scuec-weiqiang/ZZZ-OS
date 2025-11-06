/**
 * @FilePath: /ZZZ-OS/include/arch/mm.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-30 20:43:47
 * @LastEditTime: 2025-10-31 15:10:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __ASM_MM_H__
#define __ASM_MM_H__

#include <os/types.h>

typedef struct pgtbl pgtbl_t;

enum page_size {
    PAGE_SIZE_4K = 1UL << 12,
    PAGE_SIZE_2M = 1UL << 21,
    PAGE_SIZE_1G = 1UL << 30,
};

// 通用页表操作接口（各架构实现）
extern void arch_mmu_init(void);
extern pgtbl_t *arch_new_pgtbl();
extern int arch_map(pgtbl_t *pgd, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags);
extern int arch_unmap(pgtbl_t *pgd, uintptr_t va);
extern void arch_flush_pgtbl();
extern uintptr_t arch_va_to_pa(pgtbl_t *pgd, uintptr_t va);
extern void arch_switch_pgtbl(pgtbl_t *pgd);

// 通用页表接口定义
struct arch_mmu_ops {
    void (*init)(void);
    pgtbl_t *(*new)(void);
    void (*destroy)(pgtbl_t *pt);
    int  (*map)(pgtbl_t *pt, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags);
    int  (*unmap)(pgtbl_t *pt, uintptr_t va);
    uintptr_t (*translate)(pgtbl_t *pt, uintptr_t va);
    void (*flush_pgtlb)(void);
    void (*switch_pgtbl)(pgtbl_t *pgd);
};

// 由架构层实现并注册
extern struct arch_mmu_ops *arch_mmu;

#endif // __ASM_MM_H__
