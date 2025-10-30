#ifndef KERNEL_MM_H
#define KERNEL_MM_H

#include <os/types.h>
#include <asm/mm.h>

/**
 * 用户虚拟地址空间描述
 */
// struct mm_struct {
//     pgtable_t *pgtable;  // 页表指针
//     uintptr_t start_brk;
//     uintptr_t brk;
//     uintptr_t start_stack;
//     uintptr_t mmap_base;     // mmap 区域基地址
//     // struct vm_area_struct *vma_list;
// };

// extern int mm_map_range(struct mm_struct *mm, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags);
// extern int mm_unmap_range(struct mm_struct *mm, uintptr_t vaddr, size_t size);

extern pgtable_t *kernel_pgd; // 内核页全局目录
extern int map_range(pgtable_t *pgd, uintptr_t vaddr, uintptr_t paddr, size_t size, uint64_t flags);
extern int unmap_range(pgtable_t *pgd, uintptr_t vaddr, size_t size);
extern void switch_pgtable(pgtable_t *pgd);
extern void flush_pgtable();



extern void kernel_page_table_init();
extern void page_table_init(pgtable_t *pgd);

#endif