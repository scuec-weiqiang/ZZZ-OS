#include <os/pgtbl_types.h>
#include <arch/pgtbl.h>
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


inline pteval_t pa_to_pteval(phys_addr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

inline phys_addr_t pteval_to_pa(pteval_t val) {
    return (((val&0xffffffffffffffff) >> 10) << 12);
}

static inline uint32_t level_index(pgtable_t *tbl, uint32_t level, virt_addr_t va) {
    int shift = tbl->features->page_shift+ ((tbl->features->levels -1 - level) * tbl->features->level[level].bits);
    return (va >> shift) & (1<<tbl->features->level[level].bits);
}

inline void set_pte(pte_t* pte, phys_addr_t pa, uint32_t flags) {
    pteval_t val = pa_to_pteval(pa);
    pte->val = val | flags | PTE_V;
}

inline void clear_pte(pte_t* pte) {
    pte->val = 0;
}

inline int pte_valid(pte_t *pte) {
    return (pte->val & PTE_V);
}

int pte_is_leaf(pte_t *pte) {
    return (pte->val & (PTE_R | PTE_W | PTE_X)) != 0;
}

void *alloc_table() {
    return page_alloc(1);   // 一页
}

void free_table(void*p) {
    kfree(p);
}

static void* lookup_child_table(void* parent_table, uint32_t index, bool create) {
    if (parent_table == NULL)
        return NULL;
    pte_t *pte = &(((pte_t*)parent_table)[index]);
    if ((pte->val & PTE_V) == 0) {
        if (!create) {
            return NULL; // 如果不存在,但指明不需要创建就返回
        } else {
            // 否则创建对应子页表
            void* child_table = page_alloc(1);
            if (child_table == NULL)
                return NULL;
            memset(child_table, 0, PAGE_SIZE);
            set_pte(pte, KERNEL_PA(child_table), 0);
            return child_table;
        }
    } else {
        if (pte_is_leaf(pte))
        {   
            return NULL; // 叶子节点没有子表
        } else {
            return (void*)KERNEL_VA(pteval_to_pa(pte->val));
        }
    }
}

void pgtbl_flush() {
    sfence_vma();
}

phys_addr_t arch_pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    void *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->levels; i++) {
        index = level_index(pgtbl, i, va);
        pte_t *pte = &((pte_t*)table)[index];
        if (pte_is_leaf(pte))
        {
            return pteval_to_pa(pte->val);
        }  
        table = lookup_child_table(table, index, false);
    }
    return 0;
}

int arch_map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, enum huge_page big_page_size, uint32_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(va % big_page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % big_page_size == 0, "paddr is not page aligned", return -1;);

    int level,index;

    switch (big_page_size) {
    case HUGE_PAGE_4K: 
    level = 3;break;
    case HUGE_PAGE_2M:
    level = 2;break;
    case HUGE_PAGE_1G:
    level = 1;break;
    default:
        printk("Unsupported page size\n");
        return -1;
    }

    void *table = pgtbl->root;
    for (int i = 0;i < level - 1; i++) {
        index = level_index(pgtbl, i, va);
        table = lookup_child_table(table, index, true);
    }
    index = level_index(pgtbl, level-1, va);
    pte_t *pte = &((pte_t*)table)[index];
    set_pte(pte, pa, flags);
    return 0;
}

int arch_unmap(pgtable_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    void *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->levels; i++) {
        index = level_index(pgtbl, i, va);
        
        pte_t *pte = &((pte_t*)table)[index];
        if (pte_is_leaf(pte))
        {
            pte->val = 0;
            return 0; 
        }
        table = lookup_child_table(table, index, false);
    }
    return -1;
}

void pgtbl_switch_to(pgtable_t *pgtbl) {
    uintptr_t satp_val = make_satp((uintptr_t)pgtbl->root);
    satp_w((reg_t)satp_val);
}

int pgtbl_init(pgtable_t *tbl) {
    tbl->features = &riscv_features;
    tbl->root = page_alloc(1); 
    if (!tbl->root) return -1;
    memset(tbl->root, 0, PAGE_SIZE);
    return 0;
}

void pgtbl_deinit(pgtable_t *tbl) {
    tbl->features = NULL;
    
}

struct pgtable_operations riscv_pgtbl_ops = {
    .init = pgtbl_init,
    .pa_to_pteval = pa_to_pteval,
    .pteval_to_pa = pteval_to_pa,
    .level_index = level_index,
    .set_pte = set_pte,
    .clear_pte = clear_pte,
    .pte_valid = pte_valid,
    .pte_is_leaf = pte_is_leaf,
    .alloc_table = alloc_table,
    .free_table = free_table,
    .flush = pgtbl_flush,
    .switch_to = pgtbl_switch_to,
};

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


