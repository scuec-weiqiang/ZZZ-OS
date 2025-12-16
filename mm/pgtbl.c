#include "os/types.h"
#include <os/mm/pgtbl_types.h>
#include <asm/pgtbl.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/check.h>
#include <os/pfn.h>
#include <os/kva.h>
#include <os/mm/vma_flags.h>

pgtable_t *kernel_pgtbl = NULL; // kernel_page_global_directory 内核页全局目录

static void *pte_to_table(pte_t *pte) {
    phys_addr_t pa = arch_pgtbl_pteval_to_pa(pte->val);
    return (void*)KERNEL_VA(pa);
}

static void *alloc_table_va() {
    return page_alloc(1);   // 一页
}

static void free_table_va(void*p) {
    kfree(p);
}


int pgtbl_level_page_size(pgtable_t *tbl, unsigned int level) {
    if (!tbl || !tbl->features || level >= tbl->features->levels) {
        return 0;
    }
    return tbl->features->level[level].page_size;
}

pgtable_t *new_pgtbl(const char* name) {
    pgtable_t *tbl = kmalloc(sizeof(pgtable_t));
    if (!tbl) {
        return NULL;
    }

    memset(tbl, 0, sizeof(pgtable_t));
    strncpy(tbl->name, name, sizeof(tbl->name) - 1);
    tbl->asid = 0; // TODO: 分配 ASID
    if (arch_pgtbl_init(tbl) < 0) {
        kfree(tbl);
        return NULL;
    }

    return tbl;
}

int map_one(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, vma_flags_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
 
    int index;
    pte_t *table = pgtbl->root;
    pte_t *pte = NULL;
    for (int i = 0;i < target_level-1; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        pte = &table[index];

        if (arch_pgtbl_pte_valid(pte)==true) {
            if (arch_pgtbl_pte_is_leaf(pte)) {
                // 已经被映射为叶子节点，不能继续映射
                return -1;
            }
            table = pte_to_table(pte);// 还没有到最底层，继续往下找
        } else {
            // 需要创建子表
            void* child_table = alloc_table_va();
            if (child_table == NULL) {
                return -1; // 分配失败
            }
            memset(child_table, 0, PAGE_SIZE);
            arch_pgtbl_set_pte(pte, KERNEL_PA(child_table), 0);
            table = child_table;
        }
    }
    index = arch_pgtbl_level_index(pgtbl, target_level, va);
    pte = &table[index];
    arch_pgtbl_set_pte(pte, pa, flags);
    return 0;
}

int unmap_one(pgtable_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    pte_t *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->features->levels; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        pte_t *pte = &(table)[index];
        if (arch_pgtbl_pte_valid(pte)==true) {
            if (arch_pgtbl_pte_is_leaf(pte)==true) {
                arch_pgtbl_clear_pte(pte);
                return 0;
            }
            table = pte_to_table(pte);// 还没有到最底层，继续往下找
        } else {
            return -1; 
        }
    }
    return -1;
}

phys_addr_t pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    pte_t *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->features->levels; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        pte_t *pte = &table[index];
        if (arch_pgtbl_pte_valid(pte)==true) {
            if (arch_pgtbl_pte_is_leaf(pte)==true) {
                return arch_pgtbl_pteval_to_pa(pte->val);
            }
            table = pte_to_table(pte);// 还没有到最底层，继续往下找
        } else {
            return 0; 
        }
    }
    return 0;
}

void pgtbl_flush() {
    arch_pgtbl_flush();
}

void pgtbl_switch_to(pgtable_t *pgtbl) {
    arch_pgtbl_switch_to(pgtbl);
}

void pgtbl_test() {
    pgtable_t test_pgtbl = {
        .name = "test_pgtbl",
        .asid = 1,
        .root = NULL,
        .root_pa = 0,
        .features = NULL,
    };
    arch_pgtbl_init(&test_pgtbl);
 
    virt_addr_t test_va = 0x85000000UL; // KERNEL_VA_START
    phys_addr_t test_pa = 0xffffffffc5000000UL; // KERNEL_PA_BASE

    map_one(&test_pgtbl, test_va, test_pa, 3, VMA_R | VMA_W | VMA_X);
    phys_addr_t resolved_pa = pgtbl_walk(&test_pgtbl, test_va);
    printk("pa = %xu,resolved_pa = %xu\n", test_pa, resolved_pa);
    CHECK(resolved_pa == test_pa, "Page table walk failed", return;);

    unmap_one(&test_pgtbl, test_va);
    resolved_pa = pgtbl_walk(&test_pgtbl, test_va);
    printk("after umap,resolved_pa = %xu\n", resolved_pa);
    CHECK(resolved_pa == 0, "Page unmap failed", return;);

    printk("Page table test passed\n");
}