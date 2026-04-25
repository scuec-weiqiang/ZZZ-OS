#ifndef __KERNEL_VMA_H
#define __KERNEL_VMA_H

#include <os/types.h>
#include <os/bitops.h>
#include <mm/pgprot.h>
struct mm_struct;
struct vma {
    virt_addr_t start;
    virt_addr_t end;
    pgprot_t flags;
    struct mm_struct *mm; // 指向所属的mm_struct
    struct list_head node; // 链表指针
};

int vma_insert(struct mm_struct *mm, struct vma *new);
int vma_remove(struct mm_struct *mm, struct vma *vma);
struct vma *vma_create(virt_addr_t start, virt_addr_t end, pgprot_t flags);
struct vma *vma_clone(struct vma *vma);
void vma_destroy(struct vma *vma);
struct vma *vma_find(struct mm_struct *mm, virt_addr_t va);
int vma_add(struct mm_struct *mm, virt_addr_t start, size_t len, pgprot_t flags);
int vma_delete(struct mm_struct *mm, virt_addr_t start, size_t len);

#endif