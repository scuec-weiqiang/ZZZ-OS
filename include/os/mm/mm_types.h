#ifndef __KERNEL_MM_TYPES_H
#define __KERNEL_MM_TYPES_H

#include <os/types.h>
#include <os/mm/pgtbl_types.h>
#include <os/mm/vma_flags.h>
#include <os/mm/vma.h>


struct mm_struct {
    pgtable_t *pgdir;          // 进程的页表根目录
    struct list_head vma_list;      // 进程的虚拟内存区域链表
    int vma_count;           // 虚拟内存区域数量
};


#endif /* __KERNEL_MM_TYPES_H__ */