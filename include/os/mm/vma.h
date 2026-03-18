#ifndef __KERNEL_VMA_H
#define __KERNEL_VMA_H

#include <os/types.h>
#include <os/bitops.h>
#include <os/mm/pgprot.h>

typedef enum {
    VMA_NONE    = 0,         // 非pte页表项
    VMA_R       = BIT(0),    // 可读
    VMA_W       = BIT(1),    // 可写
    VMA_X       = BIT(2),    // 可执行
    VMA_U       = BIT(3),    // 用户可访问
    VMA_DIRTY   = BIT(4),
    VMA_ACCESSED= BIT(5),
    VMA_COW     = BIT(6),
} vma_flags_t;

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
pgprot_t vm_flags_to_pgprot(vma_flags_t flags);

#endif