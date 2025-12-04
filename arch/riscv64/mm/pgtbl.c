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

static inline pteval_t pa_to_pteval(phys_addr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

static inline phys_addr_t pteval_to_pa(pteval_t val) {
    return (((val&0xffffffffffffffff) >> 10) << 12);
}

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

static uint32_t alloc_asid() {
    return 0;
}

static inline uint32_t level_index(pgtbl_t *tbl, uint32_t level, virt_addr_t va) {
    int shift = tbl->page_shift + ((tbl->levels -1 - level) * 9);
    return (va >> shift) & 0x1FF;
}

static inline void set_pte(pte_t* pte, phys_addr_t pa, uint32_t flags) {
    pteval_t val = pa_to_pteval(pa);
    pte->val = val | flags | PTE_V;
}

int arch_pgtbl_init(pgtbl_t *tbl) {
    tbl->levels = 3;           // Sv39
    tbl->page_shift = 12;
    tbl->asid = alloc_asid();   // 先可以默认 0
    tbl->flags = 0;

    tbl->root = (phys_addr_t)page_alloc(1); // 4KB
    if (!tbl->root) return -1;

    memset(tbl->root, 0, PAGE_SIZE);
    return 0;
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
        return (void*)KERNEL_VA(pteval_to_pa(pte->val));
    }
}

static int is_pte_leaf(pte_t *pte) {
    return (pte->val & (PTE_R | PTE_W | PTE_X)) != 0;
}

void* arch_pgtbl_walk(pgtbl_t *pgtbl, virt_addr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    void *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->levels; i++) {
        index = level_index(pgtbl, i, va);
        table = lookup_child_table(table, index, false);
        if (is_pte_leaf(&((pte_t*)table)[index]))
        {
            return pteval_to_pa((((pte_t*)table)[index]).val);
        }
    }
    return 0;
}

int arch_map(pgtbl_t *pgtbl, virt_addr_t va, phys_addr_t pa, enum big_page big_page_size, uint32_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(va % big_page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % big_page_size == 0, "paddr is not page aligned", return -1;);

    int level,index;

    switch (big_page_size) {
    case BIG_PAGE_4K: 
    level = 3;break;
    case BIG_PAGE_2M:
    level = 2;break;
    case BIG_PAGE_1G:
    level = 1;break;
    default:
        printk("Unsupported page size\n");
        return -1;
    }

    phys_addr_t table = pgtbl->root;
    for (int i = 0;i < level - 1; i++) {
        index = level_index(pgtbl, i, va);
        table = lookup_child_table(table, index, true);
        if (!table) {
            return -1;
        }
    }
    index = level_index(pgtbl, level-1, va);
    pte_t *pte = &((pte_t*)table)[index];
    set_pte(pte, pa, flags);
    return 0;
}

int arch_unmap(pgtbl_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return -1;);

    return -1;
}

void arch_pgtbl_flush() {
    sfence_vma();
}

void arch_pgtbl_switch(pgtbl_t *pgtbl) {
    uintptr_t satp_val = make_satp((uintptr_t)pgtbl->root);
    satp_w((reg_t)satp_val);
}


