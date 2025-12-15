#include <os/mm/pgtbl_types.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/mm/page.h>
#include <os/check.h>
#include <asm/barrier.h>
#include <asm/riscv.h>  

#define KERNEL_PA_BASE 0x80000000
// #define KERNEL_VA_BASE 0xffffffffc0000000
#define KERNEL_VA_BASE 0x80000000
// #define KERNEL_VA_START 0xffffffffc0200000
#define KERNEL_VA_START 0x80400000
#define KERNEL_VA(pa) (KERNEL_VA_BASE + ((uint64_t)(pa)) - KERNEL_PA_BASE)
#define KERNEL_PA(va) ((uint64_t)(va) - KERNEL_VA_BASE + KERNEL_PA_BASE)

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


static inline reg_t make_satp(addr_t va_or_pa) {
    uintptr_t pa;

    // 如果地址在内核高地址空间，就转成物理地址
    if (va_or_pa >= KERNEL_VA_BASE) {
        pa = KERNEL_PA(va_or_pa);
    } else {
        // 否则默认就是物理地址
        pa = va_or_pa;
    }

    return SATP_MODE | (pa >> 12);
}


pteval_t arch_pgtbl_pa_to_pteval(phys_addr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

phys_addr_t arch_pteval_to_pa(pteval_t val) {
    return (((val&0xffffffffffffffff) >> 10) << 12);
}

uint32_t arch_pgtbl_level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va) {
    int shift = tbl->features->page_shift+ ((tbl->features->levels -1 - level) * tbl->features->level[level].bits);
    return (va >> shift) & (1<<tbl->features->level[level].bits);
}

void arch_pgtbl_set_pte(pte_t* pte, phys_addr_t pa, uint32_t flags) {
    pteval_t val = arch_pgtbl_pa_to_pteval(pa);
    pte->val = val | flags | PTE_V;
}

void arch_pgtbl_clear_pte(pte_t* pte) {
    pte->val = 0;
}

int arch_pgtbl_pte_valid(pte_t *pte) {
    return (pte->val & PTE_V);
}

int arch_pgtbl_pte_is_leaf(pte_t *pte) {
    return (pte->val & (PTE_R | PTE_W | PTE_X)) != 0;
}

void *arch_pgtbl_alloc_table() {
    return page_alloc(1);   // 一页
}

void arch_pgtbl_free_table(void*p) {
    kfree(p);
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
    tbl->root = page_alloc(1); 
    if (!tbl->root) return -1;
    memset(tbl->root, 0, PAGE_SIZE);
    return 0;
}

void arch_pgtbl_deinit(pgtable_t *tbl) {
    tbl->features = NULL;
}

// void arch_pgtbl_test() {
//     pgtable_t test_pgtbl;
//     arch_pgtbl_init(&test_pgtbl);

//     virt_addr_t test_va = 0x85000000UL; // KERNEL_VA_START
//     phys_addr_t test_pa = 0xffffffffc5000000UL; // KERNEL_PA_BASE

//     arch_map(&test_pgtbl, test_va, test_pa, HUGE_PAGE_4K, PTE_R | PTE_W | PTE_X);
//     phys_addr_t resolved_pa = arch_pgtbl_walk(&test_pgtbl, test_va);
//     printk("pa = %xu,resolved_pa = %xu\n", test_pa, resolved_pa);
//     CHECK(resolved_pa == test_pa, "Page table walk failed", return;);

//     arch_unmap(&test_pgtbl, test_va);
//     resolved_pa = arch_pgtbl_walk(&test_pgtbl, test_va);
//     printk("after umap,resolved_pa = %xu\n", resolved_pa);
//     CHECK(resolved_pa == 0, "Page unmap failed", return;);

//     printk("Page table test passed\n");
// }


