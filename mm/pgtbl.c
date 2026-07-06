#include "os/types.h"
#include <asm/pgtbl.h>
#include <mm/pgprot.h>
#include <mm/pgtbl.h>
#include <mm/pgtbl_types.h>
#include <mm/vma.h>
#include <os/check.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/pfn.h>
#include <os/string.h>
#include <os/utils.h>

pte_t *entry_to_next_table(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t *pte) {
    phys_addr_t pa = arch_pgtbl_entry_get_pa(tbl, level, type, pte);
    return (void *)KERNEL_VA(pa);
}

pte_t *entry_get_table_base(pgtable_t *tbl, int level, pgdesc_type_t type, pte_t *entry) {
    size_t table_size = tbl->features->level[level].table_size;
    return (pte_t *)ALIGN_DOWN((uintptr_t)entry, table_size);
}

static pte_t *alloc_table_va(pgtable_t *tbl, int level) {
    u32 npages = (tbl->features->level[level].table_size) >> PAGE_SHIFT;
    if (npages == 0) {
        npages = 1; // 最小分配一页
    }
    pte_t *p = (pte_t *)page_alloc(npages);
    // printk("alloc_table_va:%xu,level=%d,  npages=%xu\n",p, level, npages);
    return p; // 一页
}

static void free_table_va(void *p) {
    kfree(p);
}

bool pgtbl_table_is_empty(pgtable_t *pgtbl, int level, pte_t *table) {
    for (int i = 0; i < (pgtbl->features->level[level].table_size / sizeof(pte_t)); i++) {
        if (arch_pgtbl_entry_is_valid(&table[i]) == true) {
            return false;
        }
    }
    return true;
}

bool pgtbl_table_is_full(pgtable_t *pgtbl, int level, pte_t *table) {
    size_t entries = pgtbl->features->level[level].table_size / sizeof(pte_t);
    size_t valid_count = 0;
    for (int i = 0; i < entries; i++) {
        if (arch_pgtbl_entry_is_valid(&table[i]) == true) {
            valid_count++;
        }
    }
    return valid_count == entries;
}

bool pgtbl_table_need_merge(pgtable_t *pgtbl, pte_t *table, int level) {
    /*
     * User page tables shallow-copy the kernel half from init_mm.  Merging
     * frees the child table, which is unsafe if a shared kernel page-table
     * page ever reaches this path. Keep mappings split until page-table pages
     * have ownership/reference tracking.
     */
    // return false;

    if (level == 0) {
        return false; // 已经是顶层，无法合并
    }

    size_t entries = PAGE_SIZE / sizeof(pte_t);
    phys_addr_t first_pa = 0;
    pgprot_t first_flags = {0};

    for (int i = 0; i < entries; i++) {
        pte_t *pte = &table[i];
        if (arch_pgtbl_entry_is_valid(pte) == false || arch_pgtbl_entry_is_leaf(pte) == false) {
            return false; // 非法或非叶子节点，无法合并
        }

        phys_addr_t pa = arch_pgtbl_entry_get_pa(pgtbl, level, PGTBL_DESC_PAGE, pte);
        pgprot_t flags = arch_pgtbl_entry_get_flags(pgtbl, level, pte);

        if (i == 0) {
            first_pa = pa;
            first_flags = flags;
        } else {
            size_t page_size = pgtbl_level_page_size(pgtbl, level);
            if (pa != first_pa + i * page_size || flags != first_flags) {
                return false; // 物理地址不连续或权限不同，无法合并
            }
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

/*
 * 将一个叶子节点拆分为子页表
 */
void pgtbl_split(pgtable_t *pgtbl, pte_t *pte, int level) {
    if (pgtbl == NULL || pte == NULL) {
        return;
    }

    if (level >= pgtbl->features->support_levels - 1) {
        return; // 已经是最低层级，无法拆分
    }

    phys_addr_t pa = arch_pgtbl_entry_get_pa(pgtbl, level, PGTBL_DESC_PAGE, pte);
    pte_t *child_table = alloc_table_va(pgtbl, level + 1);
    if (child_table == NULL) {
        return; // 分配失败
    }
    memset(child_table, 0, PAGE_SIZE);
    arch_pgtbl_sync_range(child_table,
                          pgtbl->features->level[level + 1].table_size); // 确保新表的内容被写回内存
    size_t child_page_size = pgtbl_level_page_size(pgtbl, level + 1);
    size_t child_entries = pgtbl->features->level[level + 1].table_size / sizeof(pte_t);
    pgprot_t flags = arch_pgtbl_entry_get_flags(pgtbl, level, pte);
    for (size_t i = 0; i < child_entries; i++) {
        phys_addr_t child_pa = pa + i * child_page_size;
        // printk("pgtbl_split: set child entry %d at level %d to pa=%xu with flags %xu\n", i,
        // level+1, child_pa, flags);
        arch_pgtbl_set_entry(pgtbl, level + 1, PGTBL_DESC_PAGE, &child_table[i], child_pa,
                             flags); // 继承权限位
    }
    arch_pgtbl_sync_range(child_table, pgtbl->features->level[level + 1].table_size);

    arch_pgtbl_set_entry(pgtbl, level, PGTBL_DESC_TABLE, pte, KERNEL_PA(child_table), 0);
    arch_pgtbl_sync_range(pte, sizeof(*pte));
}

/*
 * 将一个子页表合并为叶子节点
 */
void pgtbl_merge(pgtable_t *pgtbl, int level, pte_t *table, pte_t *table_entry) {
    if (pgtbl == NULL || table == NULL) {
        return;
    }

    if (level <= 0) {
        return; // 已经是顶层，无法合并
    }

    // 确定合并的地址范围与权限
    phys_addr_t first_pa = arch_pgtbl_entry_get_pa(pgtbl, level, PGTBL_DESC_PAGE, &table[0]);
    pgprot_t flags = arch_pgtbl_entry_get_flags(pgtbl, level, &table[0]);

    pte_t *table_base = entry_get_table_base(pgtbl, level - 1, PGTBL_DESC_TABLE, table_entry);
    size_t index = (uintptr_t)table_entry - (uintptr_t)table_base;
    index /= sizeof(pte_t);

    arch_pgtbl_set_entry(pgtbl, level - 1, PGTBL_DESC_PAGE, &table_base[index], first_pa, flags);
    arch_pgtbl_sync_range(&table_base[index], sizeof(table_base[index]));

    free_table_va(table);
}

pgtable_t *new_pgtbl() {
    pgtable_t *tbl = kmalloc(sizeof(pgtable_t));
    if (!tbl) {
        return NULL;
    }

    memset(tbl, 0, sizeof(pgtable_t));

    tbl->asid = 0; // TODO: 分配 ASID
    arch_pgtbl_init(tbl);
    int npages = div_u32(tbl->features->level[0].table_size + PAGE_SIZE - 1, PAGE_SIZE);
    // printk("alloc %du pages for root\n", npages);
    tbl->root = page_alloc(npages);
    if (!tbl->root) {
        kfree(tbl);
        return NULL;
    }

    tbl->root_pa = KERNEL_PA(tbl->root);
    memset(tbl->root, 0, tbl->features->level[0].table_size);

    return tbl;
}

static void pgtbl_destroy_table_range(pgtable_t *pgtbl, pte_t *table, int level, size_t start,
                                      size_t end) {
    if (pgtbl == NULL || table == NULL || pgtbl->features == NULL) {
        return;
    }

    if (level >= pgtbl->features->support_levels - 1) {
        return;
    }

    for (size_t i = start; i < end; i++) {
        pte_t *entry = &table[i];
        pte_t *child_table;

        if (arch_pgtbl_entry_is_valid(entry) == false) {
            continue;
        }

        if (arch_pgtbl_entry_is_leaf(entry)) {
            continue;
        }

        child_table = entry_to_next_table(pgtbl, level, PGTBL_DESC_TABLE, entry);
        if (child_table == NULL) {
            continue;
        }

        pgtbl_destroy_table_range(pgtbl, child_table, level + 1, 0,
                                  pgtbl->features->level[level + 1].table_size / sizeof(pte_t));
        arch_pgtbl_clear_entry(pgtbl, level, PGTBL_DESC_TABLE, entry);
        free_table_va(child_table);
    }
}

void pgtbl_destroy(pgtable_t *pgtbl) {
    size_t root_entries;
    size_t kernel_start_index;

    if (pgtbl == NULL) {
        return;
    }

    if (pgtbl->root != NULL && pgtbl->features != NULL) {
        root_entries = pgtbl->features->level[0].table_size / sizeof(pte_t);
        kernel_start_index = arch_pgtbl_level_index(pgtbl, 0, KERNEL_VA_BASE);
        if (kernel_start_index > root_entries) {
            kernel_start_index = root_entries;
        }

        // 用户页表只销毁用户空间的页表项，保留内核空间的页表项
        pgtbl_destroy_table_range(pgtbl, (pte_t *)pgtbl->root, 0, 0, kernel_start_index);
    }

    arch_pgtbl_deinit(pgtbl);
    kfree(pgtbl);
}

struct map_ctx {
    pte_t *table[MAX_PGTBL_LEVELS];
    pte_t *pte[MAX_PGTBL_LEVELS];
    phys_addr_t pa;
    pgprot_t flags;
    int target_level;
};
walk_action_t map_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    struct map_ctx *ctx = arg;

    pgdesc_type_t type;
    if (level == ctx->target_level) {
        type = PGTBL_DESC_PAGE;
    } else {
        type = PGTBL_DESC_TABLE;
    }
    ctx->table[level] = entry_get_table_base(pgtbl, level, type, pte);
    ctx->pte[level] = pte;

    // printk("map_cb: level=%d, va=%xu, pte val=%xu\n", level, va, pte->val);
    if (level == ctx->target_level) {
        pte->val = 0; // 先清空原有映射
        // printk("map_cb: set entry at level %d, va=%xu to pa=%xu with flags %xu\n", level, va,
        // ctx->pa, ctx->flags);
        arch_pgtbl_set_entry(pgtbl, level, type, pte, ctx->pa, ctx->flags);
        arch_pgtbl_sync_range(pte, sizeof(*pte));
        for (int l = level; l > 0; l--) {
            pte_t *table = ctx->table[l];
            pte_t *table_entry = ctx->pte[l - 1];
            if (pgtbl_table_need_merge(pgtbl, table, l)) {
                // printk("map_cb: merge table at level %d\n", l);
                pgtbl_merge(pgtbl, l, table, table_entry);
                arch_pgtbl_sync_range(table_entry, sizeof(pte_t)); // 确保合并后的表项被写回内存
            } else {
                break;
            }
        }
        // printk("map success at level %d for va=%xu\n", level, va);
        return WALK_STOP;
    }

    // printk("map_cb: pte is valid=%d, is_leaf=%d at level %d for va=%xu\n",
    // arch_pgtbl_entry_is_valid(pte), arch_pgtbl_entry_is_leaf(pte), level, va);
    if (arch_pgtbl_entry_is_valid(pte) == false && type == PGTBL_DESC_TABLE) {
        // 需要创建子表
        void *child_table = alloc_table_va(pgtbl, level + 1);
        if (child_table == NULL) {
            return -1; // 分配失败
        }
        // printk("map_cb: need to create child table %xu at level %du for
        // va=%xu\n",(virt_addr_t)child_table, level, va);
        memset(child_table, 0, PAGE_SIZE);
        arch_pgtbl_set_entry(pgtbl, level, PGTBL_DESC_TABLE, pte, KERNEL_PA(child_table),
                             PROT_NONE);
        arch_pgtbl_sync_range(
            child_table, pgtbl->features->level[level + 1].table_size); // 确保新表的内容被写回内存
        arch_pgtbl_sync_range(pte, sizeof(*pte));
        // printk("create pte = %xu\n", pte->val);
    }
    return WALK_CONTINUE;
}

walk_action_t remap_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    struct map_ctx *ctx = arg;

    pgdesc_type_t type;
    if (level == ctx->target_level) {
        type = PGTBL_DESC_PAGE;
    } else {
        type = PGTBL_DESC_TABLE;
    }
    ctx->table[level] = entry_get_table_base(pgtbl, level, type, pte);
    ctx->pte[level] = pte;

    // printk("map_cb: level=%d, va=%xu, pte val=%xu\n", level, va, pte->val);
    if (level == ctx->target_level) {
        pte->val = 0; // 先清空原有映射
        // printk("map_cb: set entry at level %d, va=%xu to pa=%xu with flags %xu\n", level, va,
        // ctx->pa, ctx->flags);
        arch_pgtbl_set_entry(pgtbl, level, type, pte, ctx->pa, ctx->flags);
        arch_pgtbl_sync_range(pte, sizeof(*pte));
        for (int l = level; l > 0; l--) {
            pte_t *table = ctx->table[l];
            pte_t *table_entry = ctx->pte[l - 1];
            if (pgtbl_table_need_merge(pgtbl, table, l)) {
                // printk("map_cb: merge table at level %d\n", l);
                pgtbl_merge(pgtbl, l, table, table_entry);
                arch_pgtbl_sync_range(table_entry, sizeof(pte_t)); // 确保合并后的表项被写回内存
            } else {
                break;
            }
        }
        // printk("map success at level %d for va=%xu\n", level, va);
        return WALK_STOP;
    }

    // 映射存在且不是目标级别，直接拆分
    if (arch_pgtbl_entry_is_valid(pte) == true && arch_pgtbl_entry_is_leaf(pte) == true) {
        pgtbl_split(pgtbl, pte, level);
    }
    return WALK_CONTINUE;
}

struct look_ctx {
    phys_addr_t pa;
    pgprot_t flags;
};
walk_action_t look_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg) {
    struct look_ctx *res = (struct look_ctx *)arg;

    if (arch_pgtbl_entry_is_valid(pte) == false) {
        return WALK_STOP; // 无效映射
    }

    if (arch_pgtbl_entry_is_leaf(pte)) {
        // printk("find in level %d\n", level);
        // phys_addr_t offset = va % pgtbl_level_page_size(pgtbl, level);

        phys_addr_t offset = va % pgtbl_level_page_size(pgtbl, level);
        res->pa = arch_pgtbl_entry_get_pa(pgtbl, level, PGTBL_DESC_PAGE, pte);
        res->pa += offset;
        // printk("look_cb: pte = %xu\n", pte->val);
        res->flags = arch_pgtbl_entry_get_flags(pgtbl, level, pte);
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

    pgdesc_type_t type;
    if (level == ctx->target_level) {
        type = PGTBL_DESC_PAGE;
    } else {
        type = PGTBL_DESC_TABLE;
    }
    // 记录路径以便后续清理
    ctx->table[level] = entry_get_table_base(pgtbl, level, type, pte);
    ctx->pte[level] = pte;

    if (level == ctx->target_level) {
        arch_pgtbl_clear_entry(pgtbl, level, type, pte);
        // 向上清理空表
        for (int l = level; l > 0; l--) {
            pte_t *table_base = ctx->table[l];
            pte_t *parent_pte = ctx->pte[l - 1];

            if (pgtbl_table_is_empty(pgtbl, l, table_base)) {
                arch_pgtbl_clear_entry(pgtbl, l - 1, PGTBL_DESC_TABLE, parent_pte);
                arch_pgtbl_sync_range(parent_pte, sizeof(*parent_pte));
                free_table_va(table_base);
            } else {
                break;
            }
        }
        return WALK_STOP;
    }

    if (arch_pgtbl_entry_is_valid(pte) == false) {
        return WALK_STOP; // 映射本来就不存在，直接退出
    }

    if (arch_pgtbl_entry_is_leaf(pte) == false) {
        return WALK_CONTINUE; // 已经是非叶子节点，无需拆分
    }

    /* 
        当没达到目标层级，但属于有效的叶子节点，
       也就是说实际映射颗粒度比需要解除映射的颗粒度要大。
       则需要拆分
    */
    pgtbl_split(pgtbl, pte, level);

    return WALK_CONTINUE;
}

int pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va, int target_level, pgtbl_walk_cb cb, void *arg) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return -EINVAL;);

    pte_t *table = (pte_t *)pgtbl->root;

    unsigned long index = 0;
    for (int level = 0; level < pgtbl->features->support_levels; level++) {
        index = arch_pgtbl_level_index(pgtbl, level, va);
        pte_t *pte = &table[index];

        walk_action_t act = cb(pgtbl, pte, level, va, arg);
        // printk("walk level %d, index %xu, pte %xu\n", level, index, pte->val);
        if (act == WALK_STOP)
            return 0;

        if (act == WALK_RETRY)
            return pgtbl_walk(pgtbl, va, target_level, cb, arg);

        if (arch_pgtbl_entry_is_leaf(pte)) {
            printk("pgtbl_walk: hit leaf at level %d, but target level is %d ,problem pte = %xu\n",
                   level, target_level, pte->val);
            return -EINVAL;
        }
        table =
            entry_to_next_table(pgtbl, level, PGTBL_DESC_TABLE, pte); // 还没有到最底层，继续往下找
    }
    return 0;
}

int pgtbl_map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, pgprot_t flags) {
    struct map_ctx ctx = {
        .pa = pa,
        .flags = flags,
        .target_level = target_level,
    };
    return pgtbl_walk(pgtbl, va, target_level, map_cb, &ctx);
}

int pgtbl_remap(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level,
                pgprot_t new_flags) {
    return pgtbl_walk(pgtbl, va, target_level, remap_cb,
                      &(struct map_ctx){
                          .pa = pa,
                          .flags = new_flags,
                          .target_level = target_level,
                      });
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
        .flags = PROT_NONE,
    };
    pgtbl_walk(pgtbl, va, pgtbl->features->support_levels - 1, look_cb, &ctx);
    return ctx.pa;
}
pgprot_t pgtbl_lookup_prot(pgtable_t *pgtbl, virt_addr_t va) {
    struct look_ctx ctx = {
        .pa = 0,
        .flags = PROT_NONE,
    };
    pgtbl_walk(pgtbl, va, pgtbl->features->support_levels - 1, look_cb, &ctx);
    return ctx.flags;
}

void pgtbl_flush() {
    arch_pgtbl_flush();
}

void pgtbl_switch_to(pgtable_t *pgtbl) {
    arch_pgtbl_switch_to(pgtbl);
    arch_pgtbl_flush();
}

int pgtbl_level_index(pgtable_t *pgtbl, int level, virt_addr_t va) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
    return arch_pgtbl_level_index(pgtbl, level, va);
}

int pgtbl_copy(pgtable_t *dest, pgtable_t *src, int level, int index_start, int index_num) {
    if (!dest || !src) {
        return -1;
    }

    pte_t *src_table = src->root;
    pte_t *dest_table = dest->root;

    int index = 0;
    for (int i = 0; i < level; i++) {
        index = arch_pgtbl_level_index(src, i, index_start);
        src_table = entry_to_next_table(src, i, PGTBL_DESC_TABLE, &src_table[index]);

        index = arch_pgtbl_level_index(dest, i, index_start);
        dest_table = entry_to_next_table(dest, i, PGTBL_DESC_TABLE, &dest_table[index]);
    }

    for (int i = index_start; i < index_start + index_num; i++) {
        pte_t *pte = &src_table[i];
        if (arch_pgtbl_entry_is_valid(pte)) {
            dest_table[i] = *pte;
        }
    }

    arch_pgtbl_sync_range(&dest_table[index_start], index_num * sizeof(pte_t));

    return 0;
}
