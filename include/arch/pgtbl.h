#ifndef __ARCH_PGTBL_H
#define __ARCH_PGTBL_H

#include <os/types.h>
#include <os/pgtbl_types.h>

extern int arch_pgtbl_init(pgtbl_t *pgtbl);
extern int arch_map(pgtbl_t *pgd, uintptr_t va, uintptr_t pa, enum big_page big_page, uint32_t flags);
extern int arch_unmap(pgtbl_t *pgd, uintptr_t va);
extern void arch_pgtbl_flush();
extern uintptr_t arch_pgtbl_walk(pgtbl_t *pgd, uintptr_t va);
extern void arch_pgtbl_switch(pgtbl_t *pgd);

#endif // __ARCH_PGTBL_H