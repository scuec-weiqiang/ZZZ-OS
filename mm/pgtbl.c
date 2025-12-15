#include <os/mm/pgtbl_types.h>
#include <asm/pgtbl.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/check.h>
#include <os/pfn.h>

static void* lookup_child_table(void* parent_table, uint32_t index, bool create) {
    if (parent_table == NULL)
        return NULL;
    pte_t *pte = &(((pte_t*)parent_table)[index]);
    if ( !arch_pgtbl_pte_valid(pte) ) {
        if (!create) {
            return NULL; // 如果不存在,但指明不需要创建就返回
        } else {
            // 否则创建对应子页表
            void* child_table = page_alloc(1);
            if (child_table == NULL)
                return NULL;
            memset(child_table, 0, PAGE_SIZE);
            arch_pgtbl_set_pte(pte, KERNEL_PA(child_table), 0);
            return child_table;
        }
    } else {
        if (arch_pgtbl_pte_is_leaf(pte))
        {   
            return NULL; // 叶子节点没有子表
        } else {
            return (void*)KERNEL_VA(arch_pgtbl_pteval_to_pa(pte->val));
        }
    }
}

int map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, uint32_t flags) {
    CHECK(pgtbl != NULL, "pgtbl is NULL", return -1;);
 
    int level,index;
    level = target_level;
    void *table = pgtbl->root;
    for (int i = 0;i < level - 1; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        table = lookup_child_table(table, index, true);
    }
    index = arch_pgtbl_level_index(pgtbl, level-1, va);
    pte_t *pte = &((pte_t*)table)[index];
    arch_pgtbl_set_pte(pte, pa, flags);
    return 0;
}

int unmap(pgtable_t *pgtbl, uintptr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    void *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->levels; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        
        pte_t *pte = &((pte_t*)table)[index];
        if (arch_pgtbl_pte_is_leaf(pte))
        {
            arch_pgtbl_clear_pte(pte);
            return 0;
        }
        table = lookup_child_table(table, index, false);
    }
    return -1;
}

phys_addr_t pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va) {
    CHECK(pgtbl != NULL && pgtbl->root != NULL, "pgtbl is NULL", return 0;);

    va = ALIGN_DOWN(va, PAGE_SIZE);

    void *table = pgtbl->root;
    unsigned long index = 0;
    for (int i = 0; i < pgtbl->levels; i++) {
        index = arch_pgtbl_level_index(pgtbl, i, va);
        pte_t *pte = &((pte_t*)table)[index];
        if (arch_pgtbl_pte_is_leaf(pte))
        {
            return arch_pgtbl_pteval_to_pa(pte->val);
        }  
        table = lookup_child_table(table, index, false);
    }
    return 0;
}