
#include "os/types.h"
#include <mm/pgtbl_types.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <mm/page.h>
#include <os/check.h>
#include <asm/barrier.h>
#include <asm/riscv.h>
#include <os/kva.h>
#include <mm/pgprot.h>
#include <asm/pgtbl.h>

#define PTE_V (1 << 0)      // 有效位
#define PTE_R (1 << 1)      // 可读
#define PTE_W (1 << 2)      // 可写
#define PTE_X (1 << 3)      // 可执行
#define PTE_U (1 << 4)      // 用户模式可访问

#define SATP_NONE (0)
#define SATP_SV32 (1)
#define SATP_SV39 (8)
#define SATP_SV48 (9)

#define SATP_MODE SATP_SV39

struct satp {
    union {
        struct {
            u64 ppn : 44;        // 根页表物理页号
            u64 asid : 16;       // 地址空间标识符
            u64 mode : 4;        // 模式
        };
        u64 val;
    };
};

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
        {0, 9, SIZE_1G, SIZE_4K, "PGD"},  
        {1, 9, SIZE_2M, SIZE_4K, "PMD"},   
        {2, 9, SIZE_4K, SIZE_4K, "PTE"}, 
    },
};


static pteval_t arch_pgtbl_entry_set_pa(pgtable_t *tbl, u32 level, pgdesc_type_t type, phys_addr_t pa) {
    (void)tbl;
    (void)level;
    (void)type;
    return (pteval_t)(((pa) >> 12) << 10);
}

phys_addr_t arch_pgtbl_entry_get_pa(pgtable_t *tbl, u32 level, pgdesc_type_t type, pte_t *entry) {
    (void)tbl;
    (void)level;
    (void)type;
    return ((entry->val >> 10) << 12);
}

u32 arch_pgtbl_level_index(pgtable_t *tbl, u32 level, virt_addr_t va) {
    int shift = tbl->features->page_shift+ ((tbl->features->support_levels -1 - level) * tbl->features->level[level].bits);
    return (va >> shift) & ((1<<tbl->features->level[level].bits)-1);
}

void arch_pgtbl_entry_set_flags(pgtable_t *tbl, int level, pte_t* entry, pgprot_t flags) {
    pgprot_t _flags = 0;

    (void)tbl;
    (void)level;

    if (flags & PROT_READ) _flags |= PTE_R;
    if (flags & PROT_WRITE) _flags |= PTE_W;
    if (flags & PROT_EXEC) _flags |= PTE_X;
    if (flags & PROT_USER) _flags |= PTE_U;

    // 清除原有权限位
    entry->val &= ~(PTE_R | PTE_W | PTE_X | PTE_U);
    // 设置新的权限位
    entry->val |= _flags;
}

pgprot_t arch_pgtbl_entry_get_flags(pgtable_t *tbl, int level, pte_t* entry) {
    pgprot_t flags = 0;

    (void)tbl;
    (void)level;

    if (entry->val & PTE_R) flags |= PROT_READ;
    if (entry->val & PTE_W) flags |= PROT_WRITE;
    if (entry->val & PTE_X) flags |= PROT_EXEC;
    if (entry->val & PTE_U) flags |= PROT_USER;
    return flags;
}

void arch_pgtbl_set_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry, phys_addr_t pa, pgprot_t flags) {
    pteval_t val = arch_pgtbl_entry_set_pa(tbl, level, type, pa);
    pteval_t _flags = 0;

    if (flags & PROT_READ) _flags |= PTE_R;
    if (flags & PROT_WRITE) _flags |= PTE_W;
    if (flags & PROT_EXEC) _flags |= PTE_X;
    if (flags & PROT_USER) _flags |= PTE_U;

    entry->val = val | _flags | PTE_V;
}

void arch_pgtbl_clear_entry(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t* entry) {
    (void)tbl;
    (void)level;
    (void)type;
    entry->val = 0;
}

bool arch_pgtbl_entry_is_valid(pte_t *entry) {
    return (entry->val & PTE_V);
}

bool arch_pgtbl_entry_is_leaf(pte_t *entry) {
    return (entry->val & (PTE_R | PTE_W | PTE_X)) != 0;
}

void arch_pgtbl_sync_range(void *addr, size_t size) {
    (void)addr;
    (void)size;
    asm volatile("fence rw, rw" ::: "memory");
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
    tbl->root_pa = 0;
}
