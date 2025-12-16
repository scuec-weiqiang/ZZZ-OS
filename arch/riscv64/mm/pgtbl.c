#include "os/mm/vma_flags.h"
#include "os/types.h"
#include <os/mm/pgtbl_types.h>
#include <os/string.h>
#include <os/mm/buddy.h>
#include <os/mm/page.h>
#include <os/check.h>
#include <asm/barrier.h>
#include <asm/riscv.h> 
#include <os/kva.h>
#include <os/mm/vma_flags.h>

#define PTE_V (1 << 0)      // 有效位
#define PTE_R (1 << 1)      // 可读
#define PTE_W (1 << 2)      // 可写
#define PTE_X (1 << 3)      // 可执行
#define PTE_U (1 << 4)      // 用户模式可访问

#define PA2PTE(pa) ()
#define PTE2PA(pte) (((pte&0xffffffffffffffff) >> 10) << 12)

#define SATP_SV39 (8L << 60)
#define SATP_MODE SATP_SV39 

/* RISC-V页表特性 */
static const struct pgtable_features riscv_features = {
    .va_bits = 39,          // Sv39
    .pa_bits = 56,
    .levels = 3,
    .page_shift = 12,
    .page_size = PAGE_SIZE,
    
    .supported_page_sizes = {
        PAGE_SIZE,          // 4KB
        PAGE_SIZE * 512,    // 2MB
        PAGE_SIZE * 512 * 512, // 1GB
    },
    .num_supported_sizes = 3,
    
    .features = PGTABLE_FEATURE_HUGE_PAGES |
                PGTABLE_FEATURE_GLOBAL_PAGES |
                PGTABLE_FEATURE_USER_PAGES |
                PGTABLE_FEATURE_ACCESSED_DIRTY,
    
    .level = (struct pgtable_level[]) {
        {0, 9, PAGE_SIZE * 512 * 512, "PGD"},  // 1GB
        {1, 9, PAGE_SIZE * 512, "PMD"},        // 2MB  
        {2, 9, PAGE_SIZE, "PTE"},              // 4KB
    },
};


static inline reg_t make_satp(phys_addr_t pa) {
    return SATP_MODE | (pa >> 12);
}


pteval_t arch_pgtbl_pa_to_pteval(phys_addr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

phys_addr_t arch_pgtbl_pteval_to_pa(pteval_t  val) {
    return (((val&0xffffffffffffffff) >> 10) << 12);
}

uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va) {
    int shift = tbl->features->page_shift+ ((tbl->features->levels -1 - level) * tbl->features->level[level].bits);
    return (va >> shift) & (1<<tbl->features->level[level].bits);
}

void arch_pgtbl_set_pte(pte_t* pte, phys_addr_t pa, vma_flags_t flags) {
    pteval_t val = arch_pgtbl_pa_to_pteval(pa);
    uint32_t _flags = 0;

    if (flags & VMA_R) _flags |= PTE_R;
    if (flags & VMA_W) _flags |= PTE_W;
    if (flags & VMA_X) _flags |= PTE_X;
    if (flags & VMA_USER) _flags |= PTE_U;

    pte->val = val | _flags | PTE_V;
}

void arch_pgtbl_clear_pte(pte_t* pte) {
    pte->val = 0;
}

bool arch_pgtbl_pte_valid(pte_t *pte) {
    return (pte->val & PTE_V);
}

bool arch_pgtbl_pte_is_leaf(pte_t *pte) {
    return (pte->val & (PTE_R | PTE_W | PTE_X)) != 0;
}

bool arch_pgtbl_table_is_empty(pte_t *table) {
    for (int i = 0; i < 512; i++) {
        if (arch_pgtbl_pte_valid(&table[i])==true) {
            return false;
        }
    }
    return true;
}

void arch_pgtbl_flush() {
    sfence_vma();
}

void arch_pgtbl_switch_to(pgtable_t *pgtbl) {
    uintptr_t satp_val = make_satp((uintptr_t)pgtbl->root);
    satp_w((reg_t)satp_val);
}

int arch_pgtbl_init(pgtable_t *tbl) {
    tbl->features = &riscv_features;
    tbl->root = alloc_pages_kva(1); 
    tbl->root_pa = KERNEL_PA(tbl->root);
    if (!tbl->root) return -1;
    memset(tbl->root, 0, PAGE_SIZE);
    return 0;
}

void arch_pgtbl_deinit(pgtable_t *tbl) {
    tbl->features = NULL;
    if (tbl->root) {
        free_pages_kva(tbl->root);
        tbl->root = NULL;
    }
}




