#include <asm/mm.h>
#include <asm/pgtable.h>
#include <os/check.h>
#include <os/malloc.h>
#include <os/string.h>
#include <asm/riscv.h>

typedef uint64_t pte_t;

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

pgtable_t *arch_new_pgtable() {
    pgtable_t *pgd = (pgtable_t *)page_alloc(1);
    memset(pgd, 0, PAGE_SIZE);
    return pgd;
}

static void create_pte(pte_t *pte, uintptr_t pa, uint32_t flags) {
    *pte = PA2PTE(pa) | flags | PTE_V;
}

static pgtable_t *get_child_pgtbl(pgtable_t *parent_pgd, uint64_t vpn, bool create) {
    if (parent_pgd == NULL)
        return NULL;

    pgtable_t *child_pgd = NULL;
    if ((parent_pgd[vpn] & PTE_V) == 0) // 如果不存在,返回
    {
        if (!create) {
            return NULL; // 如果不存在,但指明不需要创建就返回
        } else           // 否则创建对应子页表
        {
            child_pgd = arch_new_pgtable();
            if (child_pgd == NULL)
                return NULL;
            create_pte(&parent_pgd[vpn], KERNEL_PA(child_pgd), 0);
            return child_pgd;
        }
        return NULL;
    } else // 如果存在
    {
        return (pgtable_t *)KERNEL_VA(PTE2PA(parent_pgd[vpn]));
    }
}

static int is_pte_leaf(pte_t pte) {
    return (pte & (PTE_R | PTE_W | PTE_X)) != 0;
}

uintptr_t arch_va_to_pa(pgtable_t *pgd, uintptr_t va) {
    CHECK(pgd != NULL, "pgd is NULL", return 0;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    va = ALIGN_DOWN(va, PAGE_SIZE);

    // enum pgt_size page_size;

    uintptr_t l2 = (uintptr_t)get_child_pgtbl(pgd, vpn2, false);
    if (is_pte_leaf(l2)) // 1GB 大页
    {
        return PTE2PA(l2);
    }

    uintptr_t l1 = (uintptr_t)get_child_pgtbl((pgtable_t *)l2, vpn1, false);
    if (is_pte_leaf(l1)) // 2MB 大页
    {
        return PTE2PA(l1);
    }

    uintptr_t l0 = (uintptr_t)get_child_pgtbl((pgtable_t *)l1, vpn0, false);
    return l0;
}

void arch_mmu_init() {
}

int arch_map_page(pgtable_t *pgd, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags) {
    CHECK(pgd != NULL, "pgd is NULL", return -1;);
    CHECK(va % page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % page_size == 0, "paddr is not page aligned", return -1;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    switch (page_size) {
    case PAGE_SIZE_4K: {
        pgtable_t *l1 = get_child_pgtbl(pgd, vpn2, true);
        pgtable_t *l0 = get_child_pgtbl(l1, vpn1, true);
        pte_t *pte = &l0[vpn0];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_2M: {
        pgtable_t *l1 = get_child_pgtbl(pgd, vpn2, true);
        pte_t *pte = &l1[vpn1];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_1G: {
        pte_t *pte = &pgd[vpn2];
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

int arch_unmap_page(pgtable_t *pgd, uintptr_t va) {

    return 0;
}

void arch_flush_tlb() {
    sfence_vma();
}

void arch_switch_pgd(pgtable_t *pgd) {
    uintptr_t satp_val = make_satp((uintptr_t)pgd);
    satp_w((reg_t)satp_val);
}