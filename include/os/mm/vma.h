#ifndef __KERNEL_VMA_H
#define __KERNEL_VMA_H
#include <os/types.h>
#include <os/mm/vma_flags.h>

struct vma {
    virt_addr_t start;
    virt_addr_t end;
    vma_flags_t flags;
    struct list_head node; // 链表指针
};

#endif