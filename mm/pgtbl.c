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

static pte_t *pte_to_table(pte_t *pte) {
    phys_addr_t pa = arch_pgtbl_pteval_to_pa(pte->val);
    return (void *)KERNEL_VA(pa);
}

static pte_t *pte_to_parent_table(pte_t *pte) {
    return (void *)ALIGN_DOWN((uintptr_t)pte, PAGE_SIZE);
}

static pte_t *alloc_table_va() {
    return (pte_t*)page_alloc(1); // 一页
}

static void free_table_va(void *p) {
    kfree(p);
}

bool pgtbl_table_is_empty(pgtable_t *pgtbl, pte_t *table) {
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


void pgtbl_split(pgtable_t *pgtbl, pte_t *pte, int level) {
    if (pgtbl == NULL || pte == NULL) {
        return;
    }

    if (level >= pgtbl->features->support_levels - 1) {
        return; // 已经是最低层级，无法拆分
    }

    if (arch_pgtbl_pte_is_leaf(pte) == false) {
        return; // 已经是非叶子节点，无需拆分
    }

    phys_addr_t pa = arch_pgtbl_pteval_to_pa(pte->val);
    pte_t *child_table = alloc_table_va();
    if (child_table == NULL) {
        return; // 分配失败
    }
    memset(child_table, 0, PAGE_SIZE);
    size_t child_page_size = pgtbl_level_page_size(pgtbl, level + 1);
    vma_flags_t flags = arch_pgtbl_pte_get_flags(pte);
    for (int i = 0; i < (PAGE_SIZE / sizeof(pte_t)); i++) {
        phys_addr_t child_pa = pa + i * child_page_size;
        arch_pgtbl_set_pte(&child_table[i], child_pa, flags); // 继承权限位
    }
 
    arch_pgtbl_set_pte(pte, KERNEL_PA(child_table), 0);
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

struct map_ctx {
    phys_addr_t pa;
    vma_flags_t flags;
    int target_level;
};
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

struct look_ctx {
    phys_addr_t pa;
};
walk_action_t look_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    phys_addr_t *res_pa = &((struct look_ctx *)arg)->pa;

    if (arch_pgtbl_pte_valid(pte) == false) {
        return WALK_STOP; // 无效映射
    }
    
    if (arch_pgtbl_pte_is_leaf(pte)) {
        phys_addr_t offset = va % pgtbl_level_page_size(pgtbl, level);
        *res_pa = arch_pgtbl_pteval_to_pa(pte->val);
        *res_pa += offset;
        return WALK_STOP;
    }
    return WALK_CONTINUE;
}

struct unmap_ctx {
    pte_t *table[MAX_PGTBL_LEVELS];
    pte_t *pte[MAX_PGTBL_LEVELS];
    virt_addr_t va;
    int target_level;
};
walk_action_t unmap_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    struct unmap_ctx *ctx = arg;

    // 记录路径以便后续清理
    ctx->table[level] = pte_to_parent_table(pte);
    ctx->pte[level] = pte;

    if (level == ctx->target_level) {
        arch_pgtbl_clear_pte(pte);
        // 向上清理空表
        for (int l = level; l > 0; l--) {
            pte_t *parent_table = ctx->table[l];
            pte_t *current_pte = ctx->pte[l];
            if (pgtbl_table_is_empty(pgtbl, parent_table)) {
                arch_pgtbl_clear_pte(current_pte);
                free_table_va(parent_table);
            } else {
                break;
            }
        }
        return WALK_STOP;
    }

    if (arch_pgtbl_pte_valid(pte) == false) {
        return WALK_STOP; // 已经不存在映射了
    }

    pgtbl_split(pgtbl, pte, level);

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

int pgtbl_unmap(pgtable_t *pgtbl, virt_addr_t va, int target_level) {
    struct unmap_ctx ctx = {
        .va = va,
        .target_level = target_level,
    };
    return pgtbl_walk(pgtbl, va, target_level, unmap_cb, &ctx);
}

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

#define SPLIT_TEST_VA  0x5000000000UL
#define SPLIT_TEST_PA  0x200000000UL

void pgtbl_split_unmap_test()
{
    pgtable_t *pgtbl = new_pgtbl("split_test_pgtbl");
    printk("[pgtbl_split_test] begin\n");

    int pte_level = pgtbl->features->support_levels - 1;
    int pmd_level = pte_level - 1;

    if (pmd_level < 0) {
        printk("[pgtbl_split_test] no PMD level, skip\n");
        return;
    }

    virt_addr_t va = ALIGN_UP(SPLIT_TEST_VA, 0x200000);
    phys_addr_t pa = ALIGN_UP(SPLIT_TEST_PA, 0x200000);

    /* ---------- 1. map 2MB ---------- */
    printk("[pgtbl_split_test] map 2MB huge page\n");
    CHECK(pgtbl_map(pgtbl, va, pa, pmd_level,
                     VMA_R | VMA_W) == 0,
          "map 2MB failed", return;);

    /* ---------- 2. 验证整个 2MB ---------- */
    for (int i = 0; i < 16; i++) {
        virt_addr_t test_va = va + i * PAGE_SIZE;
        phys_addr_t expect = pa + i * PAGE_SIZE;
        CHECK(pgtbl_lookup(pgtbl, test_va) == expect,
              "lookup before split failed", return;);
    }

    /* ---------- 3. unmap 中间一个 4KB ---------- */
    virt_addr_t hole_va = va + 0x10000; // 第 16 个 4KB
    printk("[pgtbl_split_test] unmap 4KB inside 2MB\n");

    CHECK(pgtbl_unmap(pgtbl, hole_va, pte_level) == 0,
          "split unmap failed", return;);

    /* ---------- 4. 验证 hole ---------- */
    CHECK(pgtbl_lookup(pgtbl, hole_va) == 0,
          "hole lookup should be unmapped", return;);

    /* ---------- 5. 验证 hole 前 ---------- */
    for (int i = 0; i < 8; i++) {
        virt_addr_t test_va = va + i * PAGE_SIZE;
        phys_addr_t expect = pa + i * PAGE_SIZE;
        CHECK(pgtbl_lookup(pgtbl, test_va) == expect,
              "lookup before hole failed", return;);
    }

    /* ---------- 6. 验证 hole 后 ---------- */
    for (int i = 17; i < 32; i++) {
        virt_addr_t test_va = va + i * PAGE_SIZE;
        phys_addr_t expect = pa + i * PAGE_SIZE;
        CHECK(pgtbl_lookup(pgtbl, test_va) == expect,
              "lookup after hole failed", return;);
    }

    printk("[pgtbl_split_test] PASS\n");
}

void pgtbl_test()
{
    #define TEST_VA_BASE  0x4000000000UL
    #define TEST_PA_BASE  0x100000000UL

    pgtable_t *pgtbl = new_pgtbl("test_pgtbl");
    
    printk("[pgtbl_test] begin\n");

    virt_addr_t va = TEST_VA_BASE;
    phys_addr_t pa = TEST_PA_BASE;

    /* ---------- 1. map 4K ---------- */
    printk("[pgtbl_test] map 4K\n");
    CHECK(pgtbl_map(pgtbl, va, pa,
                     pgtbl->features->support_levels - 1,
                     VMA_R | VMA_W) == 0,
          "map 4K failed", return;);

    phys_addr_t r = pgtbl_lookup(pgtbl, va);
    CHECK(r == pa, "lookup 4K failed", return;);

    /* ---------- 2. unmap 4K ---------- */
    printk("[pgtbl_test] unmap 4K\n");
    CHECK(pgtbl_unmap(pgtbl, va,
                       pgtbl->features->support_levels - 1) == 0,
          "unmap 4K failed", return;);

    CHECK(pgtbl_lookup(pgtbl, va) == 0,
          "lookup after unmap should be 0", return;);

    /* ---------- 3. map 2M (if supported) ---------- */
    if (pgtbl->features->support_levels >= 2) {
        int pmd_level = pgtbl->features->support_levels - 2;

        va = ALIGN_UP(TEST_VA_BASE + 0x200000, 0x200000);
        pa = ALIGN_UP(TEST_PA_BASE + 0x200000, 0x200000);

        printk("[pgtbl_test] map 2M\n");
        CHECK(pgtbl_map(pgtbl, va, pa,
                         pmd_level,
                         VMA_R | VMA_W) == 0,
              "map 2M failed", return;);

        /* 同一个 2M 内多个地址 */
        for (int i = 0; i < 4; i++) {
            virt_addr_t test_va = va + i * PAGE_SIZE;
            phys_addr_t expect = pa + i * PAGE_SIZE;
            CHECK(pgtbl_lookup(pgtbl, test_va) == expect,
                  "lookup inside 2M failed", return;);
        }

        printk("[pgtbl_test] unmap 2M\n");
        CHECK(pgtbl_unmap(pgtbl, va, pmd_level) == 0,
              "unmap 2M failed", return;);

        CHECK(pgtbl_lookup(pgtbl, va) == 0,
              "lookup after 2M unmap failed", return;);
    }

    /* ---------- 4. lookup unmapped ---------- */
    printk("[pgtbl_test] lookup unmapped\n");
    CHECK(pgtbl_lookup(pgtbl, TEST_VA_BASE + 0xdeadbeef) == 0,
          "lookup unmapped should return 0", return;);

    pgtbl_split_unmap_test();

    printk("[pgtbl_test] PASS\n");
}
