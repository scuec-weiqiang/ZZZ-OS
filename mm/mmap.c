#include <asm/ptrace.h>
#include <fs/binfmt.h>
#include <mm/mm_types.h>
#include <mm/pgtbl.h>
#include <mm/vma.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/pfn.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <uapi/mman_defs.h>

static int mmap_user_prot_to_kernel(int prot, pgprot_t *out)
{
    if (out == NULL) {
        return -EINVAL;
    }

    if (prot & ~(PROT_READ | PROT_WRITE | PROT_EXEC)) {
        return -EINVAL;
    }

    *out = PROT_USER | (prot & (PROT_READ | PROT_WRITE | PROT_EXEC));
    return 0;
}

static int mmap_validate_flags(int flags, int fd, long offset)
{
    int type = flags & (MAP_PRIVATE | MAP_SHARED);
    int known = MAP_PRIVATE | MAP_SHARED | MAP_FIXED | MAP_ANONYMOUS;

    if (flags & ~known) {
        return -EINVAL;
    }

    if (type != MAP_PRIVATE && type != MAP_SHARED) {
        return -EINVAL;
    }

    if ((flags & MAP_ANONYMOUS) == 0) {
        return -ENODEV;
    }

    if (fd != -1 || offset != 0) {
        return -EINVAL;
    }

    return 0;
}

static int mmap_range_valid(virt_addr_t start, size_t len)
{
    virt_addr_t end;

    if (len == 0) {
        return 0;
    }

    if (start < PAGE_SIZE) {
        return 0;
    }

    end = start + len;
    if (end < start) {
        return 0;
    }

    if (end > USER_SIGTRAMP_ADDR) {
        return 0;
    }

    return 1;
}

static int vma_range_is_free(struct mm_struct *mm, virt_addr_t start, size_t len)
{
    struct vma *vma;
    virt_addr_t end = start + len;

    if (!mmap_range_valid(start, len)) {
        return 0;
    }

    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        if (vma->end <= start) {
            continue;
        }
        if (vma->start >= end) {
            break;
        }
        return 0;
    }

    return 1;
}

static virt_addr_t find_free_range_from(struct mm_struct *mm, virt_addr_t start,
                                        size_t len)
{
    struct vma *vma;
    virt_addr_t cursor = start;

    if (!mmap_range_valid(start, len)) {
        return 0;
    }

    list_for_each_entry(vma, &mm->vma_list.node, struct vma, node) {
        if (vma->end <= cursor) {
            continue;
        }

        if (cursor + len <= vma->start) {
            return cursor;
        }

        if (vma->end > cursor) {
            cursor = ALIGN_UP(vma->end, PAGE_SIZE);
        }

        if (!mmap_range_valid(cursor, len)) {
            return 0;
        }
    }

    return mmap_range_valid(cursor, len) ? cursor : 0;
}

static virt_addr_t find_free_range(struct mm_struct *mm, virt_addr_t hint,
                                   size_t len)
{
    virt_addr_t base = ALIGN_UP(mm->brk + PAGE_SIZE, PAGE_SIZE);
    virt_addr_t addr;

    if (base < PAGE_SIZE) {
        base = PAGE_SIZE;
    }

    if (hint >= base) {
        addr = find_free_range_from(mm, hint, len);
        if (addr != 0) {
            return addr;
        }
    }

    return find_free_range_from(mm, base, len);
}

static void free_mapped_pages(struct mm_struct *mm, virt_addr_t start, size_t len)
{
    virt_addr_t va;
    virt_addr_t end = start + len;

    for (va = start; va < end; va += PAGE_SIZE) {
        phys_addr_t pa = pgtbl_lookup(mm->pgdir, va);

        if (pa != 0) {
            kfree((void *)KERNEL_VA(ALIGN_DOWN(pa, PAGE_SIZE)));
        }
    }
}

__SYSCALL__ long sys_mmap(struct pt_regs *ctx)
{
    struct mm_struct *mm = current->mm;
    virt_addr_t addr = (virt_addr_t)ctx->r[0];
    size_t len = (size_t)ctx->r[1];
    int prot = (int)ctx->r[2];
    int flags = (int)ctx->r[3];
    int fd = (int)ctx->r[4];
    long offset = (long)ctx->r[5];
    pgprot_t kprot;
    virt_addr_t target;
    unsigned long irq_flags;
    int ret;

    if (mm == NULL || mm->pgdir == NULL) {
        return -ENOMEM;
    }

    len = ALIGN_UP(len, PAGE_SIZE);
    if (len == 0) {
        return -EINVAL;
    }

    ret = mmap_user_prot_to_kernel(prot, &kprot);
    if (ret < 0) {
        return ret;
    }

    ret = mmap_validate_flags(flags, fd, offset);
    if (ret < 0) {
        return ret;
    }

    irq_flags = spin_lock_irqsave(&mm->lock);

    if (flags & MAP_FIXED) {
        if ((addr & (PAGE_SIZE - 1)) != 0) {
            ret = -EINVAL;
            goto out_unlock;
        }
        target = addr;
        if (!vma_range_is_free(mm, target, len)) {
            ret = -ENOMEM;
            goto out_unlock;
        }
    } else {
        virt_addr_t hint = addr ? ALIGN_DOWN(addr, PAGE_SIZE) : 0;

        if (hint != 0 && vma_range_is_free(mm, hint, len)) {
            target = hint;
        } else {
            target = find_free_range(mm, hint, len);
        }
        if (target == 0) {
            ret = -ENOMEM;
            goto out_unlock;
        }
    }

    ret = do_mmap(mm, target, len, kprot);
    if (ret < 0) {
        ret = ret == -EFAULT ? -ENOMEM : ret;
        goto out_unlock;
    }

    ret = (long)target;

out_unlock:
    spin_unlock_irqrestore(&mm->lock, irq_flags);
    return ret;
}

__SYSCALL__ long sys_munmap(struct pt_regs *ctx)
{
    struct mm_struct *mm = current->mm;
    virt_addr_t addr = (virt_addr_t)ctx->r[0];
    size_t len = (size_t)ctx->r[1];
    unsigned long irq_flags;
    int ret;

    if (mm == NULL || mm->pgdir == NULL) {
        return -EINVAL;
    }

    if ((addr & (PAGE_SIZE - 1)) != 0) {
        return -EINVAL;
    }

    len = ALIGN_UP(len, PAGE_SIZE);
    if (len == 0 || !mmap_range_valid(addr, len)) {
        return -EINVAL;
    }

    irq_flags = spin_lock_irqsave(&mm->lock);

    free_mapped_pages(mm, addr, len);

    ret = do_unmap(mm, addr, len);
    if (ret < 0) {
        goto out_unlock;
    }

out_unlock:
    spin_unlock_irqrestore(&mm->lock, irq_flags);
    return ret;
}

__SYSCALL__ long sys_mprotect(struct pt_regs *ctx)
{
    struct mm_struct *mm = current->mm;
    virt_addr_t addr = (virt_addr_t)ctx->r[0];
    size_t len = (size_t)ctx->r[1];
    int prot = (int)ctx->r[2];
    pgprot_t kprot;
    virt_addr_t va;
    virt_addr_t end;
    unsigned long irq_flags;
    int ret;

    if (mm == NULL || mm->pgdir == NULL) {
        return -EINVAL;
    }

    if ((addr & (PAGE_SIZE - 1)) != 0) {
        return -EINVAL;
    }

    len = ALIGN_UP(len, PAGE_SIZE);
    if (len == 0 || !mmap_range_valid(addr, len)) {
        return -EINVAL;
    }

    ret = mmap_user_prot_to_kernel(prot, &kprot);
    if (ret < 0) {
        return ret;
    }

    irq_flags = spin_lock_irqsave(&mm->lock);

    ret = vma_protect(mm, addr, len, kprot);
    if (ret < 0) {
        goto out_unlock;
    }

    end = addr + len;
    for (va = addr; va < end; va += PAGE_SIZE) {
        if (pgtbl_lookup(mm->pgdir, va) == 0) {
            continue;
        }
        if (remap(mm->pgdir, va, PAGE_SIZE, kprot) < 0) {
            ret = -EFAULT;
            goto out_unlock;
        }
    }

    ret = 0;

out_unlock:
    spin_unlock_irqrestore(&mm->lock, irq_flags);
    return ret;
}
