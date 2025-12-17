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
bool pgtbl_table_is_empty(pgtable_t *pgtbl, pte_t *table, int level); // 检查页表是否为空
void pgtbl_switch_to(pgtable_t *pgtbl);
void pgtbl_test();


struct map_ctx {
    phys_addr_t pa;
    vma_flags_t flags;
    int target_level;
};
walk_action_t map_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg);


struct look_ctx {
    phys_addr_t pa;
};
walk_action_t look_cb(pgtable_t *pgtbl, pte_t *pte, int level, virt_addr_t va, void *arg);

#endif