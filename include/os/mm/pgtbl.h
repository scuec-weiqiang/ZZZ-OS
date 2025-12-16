#ifndef __KERNEL_PGTBL_H
#define __KERNEL_PGTBL_H

#include <os/mm/pgtbl_types.h>
#include <os/mm/vma_flags.h>

extern pgtable_t *kernel_pgtbl; // kernel_page_global_directory 内核页全局目录

int pgtbl_level_page_size(pgtable_t *tbl, unsigned int level);
pgtable_t *new_pgtbl(const char* name);
int map_one(pgtable_t *pgtbl, virt_addr_t va, phys_addr_t pa, int target_level, vma_flags_t flags);
int unmap_one(pgtable_t *pgtbl, uintptr_t va);
phys_addr_t pgtbl_walk(pgtable_t *pgtbl, virt_addr_t va);
void pgtbl_flush();
void pgtbl_switch_to(pgtable_t *pgtbl);

#endif