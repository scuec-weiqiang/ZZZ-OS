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

struct mm_struct;

struct vma *vma_create(virt_addr_t start, virt_addr_t end, vma_flags_t flags);
struct vma *vma_clone(struct vma *vma);
void vma_destroy(struct vma *vma);
struct vma *vma_find(struct mm_struct *mm, virt_addr_t va);
int vma_add(struct mm_struct *mm, virt_addr_t start, size_t len, vma_flags_t flags);
int vma_delete(struct mm_struct *mm, virt_addr_t start, size_t len);

#endif