#ifndef __ASM_MM_H__
#define __ASM_MM_H__

#include <os/types.h>
#include <asm/pgtable.h>

typedef uint64_t pgtable_t;

enum page_size {
    PAGE_SIZE_4K = 1UL << 12,
    PAGE_SIZE_2M = 1UL << 21,
    PAGE_SIZE_1G = 1UL << 30,
};

// 通用页表操作接口（各架构实现）
extern void arch_mmu_init(void);
extern pgtable_t *arch_new_pgtable();
extern int arch_map_page(pgtable_t *pgd, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags);
extern int arch_unmap_page(pgtable_t *pgd, uintptr_t va);
extern void arch_flush_tlb();
extern uintptr_t arch_va_to_pa(pgtable_t *pgd, uintptr_t va);
extern void arch_switch_pgd(pgtable_t *pgd);

#endif // __ASM_MM_H__
