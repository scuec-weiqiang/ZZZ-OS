#include "os/mm/vma_flags.h"
#include "os/types.h"
#include <os/mm/pgtbl_types.h>
#include <os/string.h>
#include <os/kmalloc.h>
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
#define PTE2PA(pte) (((pte&UINT_MAX) >> 10) << 12)

#define SATP_NONE (0)
#define SATP_SV32 (1)
#define SATP_SV39 (8)
#define SATP_SV48 (9)

#ifdef RISCV64
#define SATP_MODE SATP_SV39
#else
#define SATP_MODE SATP_SV32
#endif

struct satp {
    union {
        #ifdef RISCV64
        uint64_t mode : 4;        // 模式
        uint64_t asid : 16;       // 地址空间标识符
        uint64_t ppn : 44;        // 根页表物理页号
        uint64_t val;
        #else
        uint32_t mode : 4;
        uint32_t asid : 16;
        uint32_t ppn : 12;
        uint32_t val;
        #endif
    };
    
};
#ifdef RISCV64
/* RISC-V页表特性 */
static const struct pgtable_features riscv_features = {
    .va_bits = 39,          // Sv39
    .pa_bits = 56,
    .support_levels = 3,
    .page_shift = 12,

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
#else
/* RISC-V页表特性 */
static const struct pgtable_features riscv_features = {
    .va_bits = 32,          // Sv32
    .pa_bits = 34,
    .support_levels = 2,
    .page_shift = 12,

    .features = PGTABLE_FEATURE_HUGE_PAGES |
                PGTABLE_FEATURE_GLOBAL_PAGES |
                PGTABLE_FEATURE_USER_PAGES |
                PGTABLE_FEATURE_ACCESSED_DIRTY,
    
    .level = (struct pgtable_level[]) {
        {0, 10, PAGE_SIZE * 512, "PMD"},        // 2MB  
        {1, 10, PAGE_SIZE, "PTE"},              // 4KB
    },
};

#endif

pteval_t arch_pgtbl_pa_to_pteval(phys_addr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

phys_addr_t arch_pgtbl_pteval_to_pa(pteval_t  val) {
    return ((val >> 10) << 12);
}

uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va) {
    int shift = tbl->features->page_shift+ ((tbl->features->support_levels -1 - level) * tbl->features->level[level].bits);
    return (va >> shift) & ((1<<tbl->features->level[level].bits)-1);
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

void arch_pgtbl_flush() {
    sfence_vma();
}

void arch_pgtbl_switch_to(pgtable_t *pgtbl) {
    struct satp satp;
    satp.mode = SATP_MODE; // SV39
    satp.asid = 0;
    satp.ppn = (phys_addr_t)pgtbl->root_pa >> 12;
    satp_w((reg_t)satp.val);
}

void arch_pgtbl_init(pgtable_t *tbl) {
    tbl->features = &riscv_features;
}

void arch_pgtbl_deinit(pgtable_t *tbl) {
    tbl->features = NULL;
    if (tbl->root) {
        kfree(tbl->root);
        tbl->root = NULL;
    }
}




