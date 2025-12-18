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

extern pgtable_t *kernel_pgtbl; // kernel_page_global_directory 内核页全局目录

int pgtbl_level_page_size(pgtable_t *tbl, unsigned int level);
pgtable_t *new_pgtbl(const char* name);
int pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va, int target_level, pgtbl_walk_cb cb, void *arg);
void pgtbl_flush();
bool pgtbl_table_is_empty(pgtable_t *pgtbl, pte_t *table); // 检查页表是否为空
void pgtbl_switch_to(pgtable_t *pgtbl);
void pgtbl_test();
int pgtbl_map(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, vma_flags_t flags);
int pgtbl_unmap(pgtable_t *pgtbl, virt_addr_t va, int target_level);
phys_addr_t pgtbl_lookup(pgtable_t *pgtbl, virt_addr_t va);



#endif