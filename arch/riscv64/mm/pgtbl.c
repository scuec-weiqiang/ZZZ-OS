#include <arch/pgtbl.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/mm/page.h>
#include <os/check.h>

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

static inline pteval_t pa_to_pteval(uintptr_t pa) {
    return (pteval_t)(((pa) >> 12) << 10);
}

static inline uintptr_t pteval_to_pa(pteval_t val) {
    return (((val&0xffffffffffffffff) >> 10) << 12);
}

static inline uintptr_t make_satp(uintptr_t va_or_pa) {
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

static inline int level_index(pgtbl_t *tbl, int level, uintptr_t va) {
    int shift = tbl->page_shift + (level * 9);
    return (va >> shift) & 0x1FF;
}

int arch_pgtbl_init(pgtbl_t *tbl) {
    tbl->levels = 3;           // Sv39
    tbl->page_shift = 12;
    tbl->asid = alloc_asid();   // 先可以默认 0
    tbl->flags = 0;

    tbl->root = (void *)page_alloc(1); // 4KB
    if (!tbl->root) return -1;

    memset(tbl->root, 0, PAGE_SIZE);
    return 0;
}

static void create_pte(pte_t *pte, uintptr_t pa, uint32_t flags) {
    *pte = PA2PTE(pa) | flags | PTE_V;
}

static pgd_t *get_child_pgd(pgd_t *parent_pgd, uint64_t vpn, bool create) {
    if (parent_pgd == NULL)
        return NULL;

    pgd_t *child_pgd = NULL;
    if ((parent_pgd[vpn] & PTE_V) == 0) {
        if (!create) {
            return NULL; // 如果不存在,但指明不需要创建就返回
        } else {
            // 否则创建对应子页表
            child_pgd = new_pgd();
            printk("new pgd = %xu\n",child_pgd);
            if (child_pgd == NULL)
                return NULL;
            create_pte(&parent_pgd[vpn], KERNEL_PA(child_pgd), 0);
            return child_pgd;
        }
        return NULL;
    } else {
        uintptr_t tmp = parent_pgd[vpn];
        tmp = PTE2PA(tmp);
        tmp = KERNEL_VA(tmp);
        return (pgd_t *)KERNEL_VA(PTE2PA(parent_pgd[vpn]));
    }
}

static int is_pte_leaf(pte_t pte) {
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

uintptr_t arch_walk(pgtbl_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    uint64_t vpn[3];

    vpn[0] = (va >> 30) & 0x1ff;
    vpn[1] = (va >> 21) & 0x1ff;
    vpn[2] = (va >> 12) & 0x1ff;

    va = ALIGN_DOWN(va, PAGE_SIZE);

    pgd_t *level = pgtbl->root;
    for (int i = 0; i < 3; i++) {
        level = get_child_pgd(level, vpn[i], false);
        if (is_pte_leaf((pte_t)level))
        {
            return PTE2PA((pte_t)level);
        }
    }
    return 0;
}

void arch_mmu_init() {

}

int arch_map(pgtbl_t *pgtbl, uintptr_t va, uintptr_t pa, enum big_page big_page_size, uint32_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(va % big_page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % big_page_size == 0, "paddr is not page aligned", return -1;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    switch (big_page_size) {
    case BIG_PAGE_4K: {
        pgd_t *l1 = get_child_pgd(pgtbl->root, vpn2, true);
        pgd_t *l0 = get_child_pgd(l1, vpn1, true);
        pte_t *pte = &l0[vpn0];
        create_pte(pte, pa, flags);
        break;
    }
    case BIG_PAGE_2M: {
        pgd_t *l1 = get_child_pgd(pgtbl->root, vpn2, true);
        pte_t *pte = &l1[vpn1];
        create_pte(pte, pa, flags);
        break;
    }
    case BIG_PAGE_1G: {
        pte_t *pte = &pgtbl->root[vpn2];
        create_pte(pte, pa, flags);
        break;
    }
    default: {
        printk("Unsupported page size\n");
        return -1;
    }
    }

    return 0;
}

int arch_unmap(pgtbl_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return -1;);

    uint64_t vpn[3];

    vpn[0] = (va >> 30) & 0x1ff;
    vpn[1] = (va >> 21) & 0x1ff;
    vpn[2] = (va >> 12) & 0x1ff;

    va = ALIGN_DOWN(va, PAGE_SIZE);

    pgd_t level = pgtbl->root;
    for (int i = 0; i < 3; i++) {
        level = get_child_pgd(level, vpn[i], false);
        if (is_pte_leaf((pte_t)level))
        {
            memset(level, 0, PAGE_SIZE);
            free_pgd(level);
            return 0;
        }
    }
    return -1;
}

void arch_pgtbl_flush() {
    sfence_vma();
}

void arch_pgtbl_switch(pgtbl_t *pgtbl) {
    uintptr_t satp_val = make_satp((uintptr_t)pgtbl->root);
    satp_w((reg_t)satp_val);
}


