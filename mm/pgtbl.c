#include "os/types.h"
#include <asm/pgtbl.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/mm/pgtbl_types.h>
#include <os/mm/vma_flags.h>
#include <os/pfn.h>
#include <os/string.h>
#include <os/mm/pgtbl.h>

pgtable_t *kernel_pgtbl = NULL; // kernel_page_global_directory 内核页全局目录

static void *pte_to_table(pte_t *pte) {
    phys_addr_t pa = arch_pgtbl_pteval_to_pa(pte->val);
    return (void *)KERNEL_VA(pa);
}

static void *alloc_table_va() {
    return page_alloc(1); // 一页
}

static void free_table_va(void *p) {
    kfree(p);
}

bool papgtbl_table_is_empty(pgtable_t *pgtbl, pte_t *table, int level) {
    for (int i = 0; i < (PAGE_SIZE / sizeof(pte_t)); i++) {
        if (arch_pgtbl_pte_valid(&table[i]) == true) {
            return false;
        }
    }
    return true;
}

int pgtbl_level_page_size(pgtable_t *tbl, unsigned int level) {
    if (!tbl || !tbl->features || level >= tbl->features->support_levels) {
        return 0;
    }
    return tbl->features->level[level].page_size;
}

pgtable_t *new_pgtbl(const char *name) {
    pgtable_t *tbl = kmalloc(sizeof(pgtable_t));
    if (!tbl) {
        return NULL;
    }

    memset(tbl, 0, sizeof(pgtable_t));
    strncpy(tbl->name, name, sizeof(tbl->name) - 1);
    tbl->asid = 0; // TODO: 分配 ASID
    arch_pgtbl_init(tbl);
    tbl->root = page_alloc(1);
    if (!tbl->root)
        return NULL;
    tbl->root_pa = KERNEL_PA(tbl->root);
    memset(tbl->root, 0, PAGE_SIZE);

    return tbl;
}

walk_action_t map_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    struct map_ctx *ctx = arg;
 
    if (level == ctx->target_level) {
        arch_pgtbl_set_pte(pte, ctx->pa, ctx->flags);
        return WALK_STOP;
    }

    if (arch_pgtbl_pte_valid(pte) == false) {
            // 需要创建子表
        void *child_table = alloc_table_va();
        if (child_table == NULL) {
            return -1; // 分配失败
        }
        memset(child_table, 0, PAGE_SIZE);
        arch_pgtbl_set_pte(pte, KERNEL_PA(child_table), 0);
    }
    return WALK_CONTINUE;
}

walk_action_t look_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    phys_addr_t *res_pa = &((struct look_ctx *)arg)->pa;

    if (arch_pgtbl_pte_is_leaf(pte)) {
        *res_pa = arch_pgtbl_pteval_to_pa(pte->val);
        return WALK_STOP;
    }
    return WALK_CONTINUE;
}

int pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va, int target_level, pgtbl_walk_cb cb, void *arg) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    pte_t *table = pgtbl->root;
    unsigned long index = 0;
    for (int level = 0; level < pgtbl->features->support_levels; level++) {
        index = arch_pgtbl_level_index(pgtbl, level, va);
        pte_t *pte = &table[index];

        walk_action_t act = cb(pgtbl, pte, level, va, arg);

        if (act == WALK_STOP)
            return 0;

        if (act == WALK_RETRY)
            return pgtbl_walk(pgtbl, va, target_level, cb, arg);
    
        if (arch_pgtbl_pte_is_leaf(pte)) {
            return -1;
        }
        table = pte_to_table(pte); // 还没有到最底层，继续往下找

    }
    return 0;
}

int pgtbl_map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, vma_flags_t flags) {
    struct map_ctx ctx = {
        .pa = pa,
        .flags = flags,
        .target_level = target_level,
    };
    return pgtbl_walk(pgtbl, va, target_level, map_cb, &ctx);
}

// void pgtbl_unmap(pgtable_t *pgtbl, virt_addr_t va, int target_level) {
//     struct unmap_ctx ctx = {
//         .va = va,
//         .target_level = target_level,
//     };
//     pgtbl_walk(pgtbl, va, target_level, unmap_cb, &ctx);
// }

phys_addr_t pgtbl_lookup(pgtable_t *pgtbl, virt_addr_t va) {
    struct look_ctx ctx = {
        .pa = 0,
    };
    pgtbl_walk(pgtbl, va, 2, look_cb, &ctx);
    return ctx.pa;
}

void pgtbl_flush() {
    arch_pgtbl_flush();
}

void pgtbl_switch_to(pgtable_t *pgtbl) {
    arch_pgtbl_switch_to(pgtbl);
}

void pgtbl_test() {
    pgtable_t *test_pgtbl = new_pgtbl("test_pgtbl");

    virt_addr_t test_pa = 0x85000000UL;         // KERNEL_VA_START
    phys_addr_t test_va = 0xffffffffc5000000UL; // KERNEL_PA_BASE

    // map_one(&test_pgtbl, test_va, test_pa, 2, VMA_R | VMA_W | VMA_X);
    struct map_ctx ctx = {
        .pa = test_pa,
        .flags = VMA_R | VMA_W | VMA_X,
        .target_level = 2,
    };
    pgtbl_walk(test_pgtbl, test_va, 2, map_cb, &ctx);

    phys_addr_t resolved_pa = 0;
    pgtbl_walk(test_pgtbl, test_va, 2, look_cb, &resolved_pa);

    printk("pa = %xu,resolved_pa = %xu\n", test_pa, resolved_pa);
    
    // CHECK(resolved_pa == test_pa, "Page table walk failed", return;);

    // unmap_one(&test_pgtbl, test_va);
    // resolved_pa = pgtbl_walk(&test_pgtbl, test_va);
    // printk("after umap,resolved_pa = %xu\n", resolved_pa);
    // CHECK(resolved_pa == 0, "Page unmap failed", return;);

    // printk("Page table test passed\n");
    while (1) {

    }
}