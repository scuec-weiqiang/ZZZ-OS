#include <os/mm/vma.h>
#include <os/mm/mm_types.h>
#include <os/list.h>
#include <os/kmalloc.h>

struct vma *vma_create(virt_addr_t start, virt_addr_t end, vma_flags_t flags) {
    struct vma *vma = kmalloc(sizeof(struct vma));
    if (!vma) {
        return NULL;
    }
    vma->start = start;
    vma->end = end;
    vma->flags = flags;
    INIT_LIST_HEAD(&vma->node);
    return vma;
}

struct vma *vma_clone(struct vma *vma) {
    return vma_create(vma->start, vma->end, vma->flags);
}

void vma_destroy(struct vma *vma) {
    kfree(vma);
}

struct vma *vma_find(struct mm_struct *mm, virt_addr_t va) {
    struct vma *vma;
    list_for_each_entry(vma, &mm->vma_list, struct vma, node) {
        if (vma->start <= va && vma->end > va) {
            return vma;
        }
    }
    return NULL;
}

int vma_insert(struct mm_struct *mm, struct vma *vma) {
    
    return 0;
}


int vma_remove(struct mm_struct *mm, virt_addr_t start, size_t len) {
    
}
