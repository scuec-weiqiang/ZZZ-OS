#include <os/check.h>
#include <os/kmalloc.h>
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
}pgtbl_t;


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
    memset(pgd, 0, PAGE_SIZE);
    return pgd;
}

static void free_pgd(pgd_t *pgd) {
    if (pgd) {
        kfree((void*)pgd);
    }
}

pgtbl_t *arch_new_pgtbl() {
    pgtbl_t *pgtbl = (pgtbl_t *)kmalloc(sizeof(pgtbl_t));
    pgtbl->root = new_pgd();
    return pgtbl;
}

void arch_destroy_pgtbl(pgtbl_t *pgtbl) {
    if (pgtbl) {
        if (pgtbl->root) {
            kfree((void*)pgtbl->root);
        }
        kfree(pgtbl);
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

uintptr_t arch_va_to_pa(pgtbl_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

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
            return PTE2PA(level);
        }
    }
    return 0;
}

void arch_mmu_init() {

}

int arch_map(pgtbl_t *pgtbl, uintptr_t va, uintptr_t pa, enum page_size page_size, uint32_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    CHECK(va % page_size == 0, "vaddr is not page aligned", return -1;);
    CHECK(pa % page_size == 0, "paddr is not page aligned", return -1;);

    uint64_t vpn2 = (va >> 30) & 0x1ff;
    uint64_t vpn1 = (va >> 21) & 0x1ff;
    uint64_t vpn0 = (va >> 12) & 0x1ff;

    switch (page_size) {
    case PAGE_SIZE_4K: {
        pgd_t *l1 = get_child_pgd(pgtbl->root, vpn2, true);
        pgd_t *l0 = get_child_pgd(l1, vpn1, true);
        pte_t *pte = &l0[vpn0];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_2M: {
        pgd_t *l1 = get_child_pgd(pgtbl->root, vpn2, true);
        pte_t *pte = &l1[vpn1];
        create_pte(pte, pa, flags);
        break;
    }
    case PAGE_SIZE_1G: {
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

void arch_flush_pgtbl() {
    sfence_vma();
}

void arch_switch_pgtbl(pgtbl_t *pgtbl) {
    uintptr_t satp_val = make_satp((uintptr_t)pgtbl->root);
    satp_w((reg_t)satp_val);
}

void arch_pgtbl_test () {
    printk("\n==== PAGETABLE TEST BEGIN ====\n");

    pgtbl_t *pgtbl = arch_new_pgtbl();
    if (!pgtbl) {
        panic("new pgtbl failed");
    }

    uintptr_t va1 = 0x40000000; // 1GB 对齐
    uintptr_t pa1 = 0x20000000; // 1GB 对齐

    uintptr_t va2 = 0x40400000; // 2MB 对齐
    uintptr_t pa2 = 0x20400000; // 2MB 对齐

    uintptr_t va3 = 0x40401000; // 4KB 对齐
    uintptr_t pa3 = 0x20401000; // 4KB 对齐

    // 映射 1GB 大页
    if (arch_map(pgtbl, va1, pa1, PAGE_SIZE_1G, PTE_R | PTE_W) < 0) {
        panic("map 1G page failed");
    }
    // 映射 2MB 大页
    if (arch_map(pgtbl, va2, pa2, PAGE_SIZE_2M, PTE_R | PTE_W) < 0) {
        panic("map 2M page failed");
    }
    // 映射 4KB 页
    if (arch_map(pgtbl, va3, pa3, PAGE_SIZE_4K, PTE_R | PTE_W) < 0) {
        panic("map 4K page failed");
    }

    // 测试地址转换
    if (arch_va_to_pa(pgtbl, va1) != pa1) {
        panic("va to pa translation failed for 1G page");
    }
    if (arch_va_to_pa(pgtbl, va2) != pa2) {
        panic("va to pa translation failed for 2M page");
    }
    if (arch_va_to_pa(pgtbl, va3) != pa3) {
        panic("va to pa translation failed for 4K page");
    }

    printk("Page table test passed!\n");

    arch_destroy_pgtbl(pgtbl);

    printk("==== PAGETABLE TEST END ====\n\n");
}
