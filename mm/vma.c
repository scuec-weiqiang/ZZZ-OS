#include <mm/vma.h>
#include <mm/mm_types.h>
#include <os/list.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/err.h>

#define node_to_vma(node_ptr) list_entry(node_ptr, struct vma, node)

struct vma *vma_create(virt_addr_t start, virt_addr_t end, pgprot_t flags) {
    struct vma *vma = kmalloc(sizeof(struct vma));
    if (!vma) {
        return ERR_PTR(-ENOMEM);
    }
    vma->start = start;
    vma->end = end;
    vma->flags = flags;
    INIT_LIST_HEAD(&vma->node);

    vma->mm = NULL; // 需要在插入到mm_struct时设置
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
    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        if (vma->start <= va && vma->end > va && vma->flags) {
            return vma;
        }
    }
    return ERR_PTR(-EFAULT);
}

static int vma_split(struct mm_struct *mm, struct vma *vma, virt_addr_t split_addr) {
    if (!mm || !vma) {
        return -1;
    }

    if (split_addr <= vma->start || split_addr >= vma->end) {
        // No need to split
        return 0;
    }

    struct vma *new_vma = vma_create(split_addr, vma->end, vma->flags);
    if (!new_vma) {
        return -ENOMEM;
    }

    vma->end = split_addr;
    list_add_after(&vma->node, &new_vma->node);
    return vma->end - vma->start;
}

static int vma_merge(struct mm_struct *mm, struct vma *vma1, struct vma *vma2) {
    if (!mm || !vma1 || !vma2) {
        return -EINVAL;
    }

    if (vma1->end != vma2->start || vma1->flags != vma2->flags) {
        return -EINVAL; // Cannot merge
    }

    vma1->end = vma2->end;
    list_del(&vma2->node);
    vma_destroy(vma2);
    return 0;
}

int vma_insert(struct mm_struct *mm, struct vma *new) {
    struct list_head *head;
    struct vma *pos;

    if (!mm || !new) {
        return -EINVAL;
    }
    head = &mm->vma_list.node;
    new->mm = mm;

    if (list_empty(head)) {
        list_add_after(head, &new->node);
    } else {
        list_for_each_entry(pos, head, struct vma, node) {
            if (new->end <= pos->start) {
                list_add_before(&pos->node, &new->node);
                goto merged;
            }
            if (new->start < pos->end) {
                return -EFAULT;
            }
        }

        list_add_tail(head, &new->node);
    }

merged:
    if (new->node.prev != head) {
        struct vma *prev = list_entry(new->node.prev, struct vma, node);
        if (vma_merge(mm, prev, new) == 0) {
            new = prev;
        }
    }

    if (new->node.next != head) {
        struct vma *next = list_entry(new->node.next, struct vma, node);
        vma_merge(mm, new, next);
    }

    return 0;
}

int vma_remove(struct mm_struct *mm, struct vma *vma) {
    if (!mm || !vma) {
        return -EINVAL;
    }

    list_del(&vma->node);
    vma_destroy(vma);

    return 0;
}

int vma_add(struct mm_struct *mm, virt_addr_t start, size_t len, pgprot_t flags) {
    if (!mm || len == 0) {
        return -EINVAL;
    }

    // 有重叠直接报错
    struct vma *tmp = vma_find(mm, start);
    if (!IS_ERR(tmp)) {
        return -EFAULT;
    }

    struct vma *new_vma = vma_create(start, start + len, flags);
    if (!new_vma) {
        return -ENOMEM;
    }
    int ret = vma_insert(mm, new_vma);
    if (ret != 0) {
        vma_destroy(new_vma);
        return ret;
    }
    return 0;
}

int vma_delete(struct mm_struct *mm, virt_addr_t start, size_t len) {
    if (!mm || len == 0) {
        return -EINVAL;
    }

    virt_addr_t end = start + len;
    virt_addr_t addr = start;
    struct vma *vma = node_to_vma(mm->vma_list.node.next);

    // 这一步是将所有和删除范围有交集的vma进行拆分，拆分成多个vma，每个vma要么完全在删除范围内，要么完全在删除范围外
    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        if (vma->end <= addr) {
            // 当前vma在删除范围之前，跳过
            continue;
        }
        if (vma->start >= end) {
            // 当前vma在删除范围之后，结束
            break;
        }
        
        if (vma->start == addr) 
        {
            addr += len;
        }

        if (vma_split(mm, vma, addr) < 0) {
            return -1;
        }

        addr += vma->end - vma->start;
    }

    // 删除所有完全在删除范围内的vma
    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        if (vma->end <= start) {
            // 当前vma在删除范围之前，跳过
            continue;
        }
        if (vma->start >= end) {
            break;
            // 当前vma在删除范围之后，结束      
        }
        if (vma->start >= start && vma->end <= end) {
            // 当前vma完全在删除范围内，删除
            struct vma *to_delete = vma;
            vma = node_to_vma(vma->node.prev); // 先保存前一个节点，防止删除后无法继续遍历
            vma_remove(mm, to_delete);
        }
    }
    return 0;
}


void vma_dump(struct mm_struct *mm) {
    struct vma *vma;
    printk("VMA Dump:\n");
    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        printk("VMA: start=%x, end=%x, flags=%x\n", vma->start, vma->end, vma->flags);
    }
}
