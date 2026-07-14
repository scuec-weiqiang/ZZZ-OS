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
    list_for_each_entry(vma, &mm->vma_list.node, node) {
        if (vma->start <= va && vma->end > va && vma->flags) {
            return vma;
        }
    }
    return ERR_PTR(-EFAULT);
}

static struct vma *vma_split(struct mm_struct *mm, struct vma *vma,
                             virt_addr_t split_addr)
{
    struct vma *new_vma;

    if (!mm || !vma) {
        return ERR_PTR(-EINVAL);
    }

    if (split_addr <= vma->start || split_addr >= vma->end) {
        return vma;
    }

    new_vma = vma_create(split_addr, vma->end, vma->flags);
    if (IS_ERR(new_vma)) {
        return new_vma;
    }

    new_vma->mm = mm;
    vma->end = split_addr;
    list_add_after(&vma->node, &new_vma->node);
    return new_vma;
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
        list_for_each_entry(pos, head, node) {
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

static void vma_merge_all(struct mm_struct *mm)
{
    struct list_head *head;
    struct list_head *pos;

    if (!mm) {
        return;
    }

    head = &mm->vma_list.node;
    pos = head->next;
    while (pos != head && pos->next != head) {
        struct vma *curr = list_entry(pos, struct vma, node);
        struct vma *next = list_entry(pos->next, struct vma, node);

        if (vma_merge(mm, curr, next) == 0) {
            continue;
        }
        pos = pos->next;
    }
}

static bool vma_range_is_mapped(struct mm_struct *mm, virt_addr_t start,
                                virt_addr_t end)
{
    struct vma *vma;
    virt_addr_t cursor = start;

    if (start >= end) {
        return false;
    }

    list_for_each_entry(vma, &mm->vma_list.node, node) {
        if (vma->end <= cursor) {
            continue;
        }
        if (vma->start > cursor) {
            return false;
        }
        if (vma->end >= end) {
            return true;
        }
        cursor = vma->end;
    }

    return false;
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
    struct list_head *head;
    struct vma *vma;
    struct vma *next;
    virt_addr_t end;

    if (!mm || len == 0) {
        return -EINVAL;
    }

    end = start + len;
    if (end < start) {
        return -EINVAL;
    }

    head = &mm->vma_list.node;
    list_for_each_entry(vma, head, node) {
        if (vma->end <= start) {
            continue;
        }
        if (vma->start >= end) {
            break;
        }
        if (vma->start < start) {
            vma = vma_split(mm, vma, start);
            if (IS_ERR(vma)) {
                return PTR_ERR(vma);
            }
        }
        break;
    }

    list_for_each_entry(vma, head, node) {
        if (vma->end <= end) {
            continue;
        }
        if (vma->start < end) {
            if (IS_ERR(vma_split(mm, vma, end))) {
                return -ENOMEM;
            }
        }
        break;
    }

    list_for_each_entry_safe(vma, next, head, node) {
        if (vma->end <= start) {
            continue;
        }
        if (vma->start >= end) {
            break;
        }
        vma_remove(mm, vma);
    }
    return 0;
}

int vma_protect(struct mm_struct *mm, virt_addr_t start, size_t len,
                pgprot_t flags)
{
    struct list_head *head;
    struct vma *vma;
    virt_addr_t end;

    if (!mm || len == 0) {
        return -EINVAL;
    }

    end = start + len;
    if (end < start) {
        return -EINVAL;
    }

    if (!vma_range_is_mapped(mm, start, end)) {
        return -ENOMEM;
    }

    head = &mm->vma_list.node;
    list_for_each_entry(vma, head, struct vma, node) {
        if (vma->end <= start) {
            continue;
        }
        if (vma->start >= end) {
            break;
        }
        if (vma->start < start) {
            vma = vma_split(mm, vma, start);
            if (IS_ERR(vma)) {
                return PTR_ERR(vma);
            }
        }
        break;
    }

    list_for_each_entry(vma, head, node) {
        if (vma->end <= end) {
            continue;
        }
        if (vma->start < end) {
            if (IS_ERR(vma_split(mm, vma, end))) {
                return -ENOMEM;
            }
        }
        break;
    }

    list_for_each_entry(vma, head, node) {
        if (vma->end <= start) {
            continue;
        }
        if (vma->start >= end) {
            break;
        }
        vma->flags = flags;
    }

    vma_merge_all(mm);

    return 0;
}


void vma_dump(struct mm_struct *mm) {
    struct vma *vma;
    printk("VMA Dump:\n");
    list_for_each_entry(vma, &mm->vma_list.node, node) {
        printk("VMA: start=%x, end=%x, flags=%x\n", vma->start, vma->end, vma->flags);
    }
}
