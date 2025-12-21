#include <os/mm/vma.h>
#include <os/mm/mm_types.h>
#include <os/list.h>
#include <os/kmalloc.h>
#include <os/printk.h>

#define node_to_vma(node_ptr) list_entry(node_ptr, struct vma, node)

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
        return -1; // Memory allocation failed
    }

    vma->end = split_addr;
    list_add_after(&vma->node, &new_vma->node);
    return vma->end - vma->start;
}

static int vma_merge(struct mm_struct *mm, struct vma *vma1, struct vma *vma2) {
    if (!mm || !vma1 || !vma2) {
        return -1;
    }

    if (vma1->end != vma2->start || vma1->flags != vma2->flags) {
        return -1; // Cannot merge
    }

    vma1->end = vma2->end;
    list_del(&vma2->node);
    vma_destroy(vma2);
    return 0;
}

static int vma_insert(struct mm_struct *mm, struct vma *new) {
    if (!mm || !new) {
        return -1;
    }

    /* 如果直接在遍历时插入，我们需要考虑在哪里插入的问题
     但是这样会导致插入时需要考虑前后关系，代码复杂度增加。
     例如：链表为空，直接插入即可；如果只有一个节点，我们要插入的节点可能在这个节点之后，也可能在这个节点之前，
     我们需要单独处理这些情况，就多了许多if判断，难以套用通用逻辑
     
     这里我们先简单地在头部插入，也就是假设插入的地址是最小的，如果实际它大于其他节点的起始地址，我们只需要把他移到这个节点之后即可
     这样可以简化插入逻辑
    */

    if (list_empty(&mm->vma_list)) {
        list_add_after(&mm->vma_list, &new->node);
        return 0;
    }

    struct vma *pos;
    list_for_each_entry_prev(pos, &mm->vma_list, struct vma, node) {
        if (pos->end <= new->start) {
            list_add_after(&pos->node, &new->node);
            vma_merge(mm, pos, new);
            break;
        }
    }
    return 0;
}

static int vma_remove(struct mm_struct *mm, struct vma *vma) {
    if (!mm || !vma) {
        return -1;
    }

    list_del(&vma->node);
    vma_destroy(vma);

    return 0;
}

int vma_add(struct mm_struct *mm, virt_addr_t start, size_t len, vma_flags_t flags) {
    if (!mm || len == 0) {
        return -1;
    }

    // 有重叠直接报错
    struct vma *tmp = vma_find(mm, start);
    if (tmp) {
        return -1;
    }

    struct vma *new_vma = vma_create(start, start + len, flags);
    if (!new_vma) {
        return -1;
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
        return -1;
    }

    virt_addr_t end = start + len;
    virt_addr_t addr = start;
    struct vma *vma = node_to_vma(mm->vma_list.next);

    // 这一步是将所有和删除范围有交集的vma进行拆分，拆分成多个vma，每个vma要么完全在删除范围内，要么完全在删除范围外
    list_for_each_entry(vma, &mm->vma_list, struct vma, node) {
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
    list_for_each_entry(vma, &mm->vma_list, struct vma, node) {
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
    list_for_each_entry(vma, &mm->vma_list, struct vma, node) {
        printk("VMA: start=%x, end=%x, flags=%x\n", vma->start, vma->end, vma->flags);
    }
}

void vma_test() {
    struct mm_struct mm;
    INIT_LIST_HEAD(&mm.vma_list);

    vma_add(&mm, 0, 1, 0);
    vma_dump(&mm);
    vma_add(&mm, 0x1000, 0x1000, VMA_R | VMA_W);
    vma_dump(&mm);
    vma_add(&mm, 0x2000, 0x1000, VMA_R | VMA_W); 
    vma_dump(&mm);
    vma_add(&mm, 0x3000, 0x1000, VMA_R);
    vma_dump(&mm);
    vma_delete(&mm, 0x1400, 0x1440);
    vma_dump(&mm);
}


