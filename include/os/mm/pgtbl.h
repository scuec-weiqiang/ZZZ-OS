#ifndef __KERNEL_PGTBL_H
#define __KERNEL_PGTBL_H

#include <os/mm/pgtbl_types.h>
#include <os/mm/vma_flags.h>

typedef enum {
    WALK_CONTINUE,
    WALK_STOP,
    WALK_RETRY, // split 后重走
} walk_action_t;

typedef walk_action_t (*pgtbl_walk_cb)(
    pgtable_t *pgtbl,
    pte_t *pte,
    int level,
    virt_addr_t va,
    void *arg);

int pgtbl_level_page_size(pgtable_t *tbl, unsigned int level);
bool pgtbl_table_is_empty(pgtable_t *pgtbl, pte_t *table); // 检查页表是否为空
int pgtbl_level_index(pgtable_t *pgtbl, int level, virt_addr_t va);
int pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va, int target_level, pgtbl_walk_cb cb, void *arg);

pgtable_t *new_pgtbl(const char* name);
int pgtbl_copy(pgtable_t *dest, pgtable_t *src, int level, int index_start, int index_num);
int pgtbl_map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, vma_flags_t flags);
int pgtbl_unmap(pgtable_t *pgtbl, virt_addr_t va, int target_level);
phys_addr_t pgtbl_lookup(pgtable_t *pgtbl, virt_addr_t va);
void pgtbl_switch_to(pgtable_t *pgtbl);
void pgtbl_flush();

void pgtbl_test();

#endif