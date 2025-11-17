#include <os/check.h>
#include <os/malloc.h>
#include <os/string.h>
#include <asm/mm.h>
#include <asm/barrier.h>
#include <asm/riscv.h>
#include <asm/pgtbl.h>
#include <os/pfn.h>
#include <os/mm.h>

typedef uintptr_t pte_t;
typedef uintptr_t pgd_t;

typedef struct pgtbl {
    pgd_t *root;
}pgtbl;


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

static pgd_t *new_pgd() {
    pgd_t *pgd = (pgd_t*)page_alloc(1);
    return pgd;
}

pgtbl_t *arch_new_pgtbl() {
    pgtbl_t *pgtbl = (pgtbl_t *)malloc(sizeof(pgtbl_t));
    pgtbl->root = new_pgd();
    memset(pgtbl->root, 0, PAGE_SIZE);
    return pgtbl;
}

void arch_destroy_pgtbl(pgtbl_t *pgtbl) {
    if (pgtbl) {
        if (pgtbl->root) {
            free((void*)pgtbl->root);
        }
        free(pgtbl);
    }
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
            if (child_pgd == NULL)
                return NULL;
            create_pte(&parent_pgd[vpn], KERNEL_PA(child_pgd), 0);
            return child_pgd;
        }
        return NULL;
    } else {
        return (pgd_t *)KERNEL_VA(PTE2PA(parent_pgd[vpn]));
    }
}

static int is_pte_leaf(pte_t pte) {
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

uintptr_t arch_va_to_pa(pgtbl_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    va = ALIGN_DOWN(va, PAGE_SIZE);

    // enum pgt_size page_size;

    uintptr_t l2 = (uintptr_t)get_child_pgd(pgtbl->root, vpn2, false);
    if (is_pte_leaf(l2)) // 1GB 大页
    {
        return PTE2PA(l2);
    }

    uintptr_t l1 = (uintptr_t)get_child_pgd((pgd_t *)l2, vpn1, false);
    if (is_pte_leaf(l1)) // 2MB 大页
    {
        return PTE2PA(l1);
    }

    uintptr_t l0 = (uintptr_t)get_child_pgd((pgd_t *)l1, vpn0, false);
    return l0;
}

void arch_mmu_init() {

}

int arch_map(pgtbl_t *pgd, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags) {
    CHECK(pgd != NULL, "pgd is NULL", return -1;);
    CHECK(va % page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % page_size == 0, "paddr is not page aligned", return -1;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    switch (page_size) {
    case PAGE_SIZE_4K: {
        pgd_t *l1 = get_child_pgd(pgd->root, vpn2, true);
        pgd_t *l0 = get_child_pgd(l1, vpn1, true);
        pte_t *pte = &l0[vpn0];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_2M: {
        pgd_t *l1 = get_child_pgd(pgd->root, vpn2, true);
        pte_t *pte = &l1[vpn1];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_1G: {
        pte_t *pte = &pgd->root[vpn2];
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

int arch_unmap(pgtbl_t *pgd, uintptr_t va) {

    return 0;
}

void arch_flush_pgtbl() {
    sfence_vma();
}

void arch_switch_pgtbl(pgtbl_t *pgd) {
    uintptr_t satp_val = make_satp((uintptr_t)pgd->root);
    satp_w((reg_t)satp_val);
}
