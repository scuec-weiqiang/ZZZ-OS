#include <mm/vma.h>
#include <mm/mm_types.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/pfn.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/string.h>
#include <os/err.h>
#include <os/spinlock.h>
#include <mm/pgtbl.h>

int do_page_fault(struct mm_struct *mm, virt_addr_t fault_addr, int fault_flags) {
    virt_addr_t page_va;
    struct vma *vma;
    void *kva;
    unsigned long flags;
    int ret = -1;
    
    if (!mm || mm->pgdir == NULL) {
        return -1;
    }

    flags = spin_lock_irqsave(&mm->lock);

    vma = vma_find(mm, fault_addr);
    if (IS_ERR(vma)) {
        goto out_unlock;
    }

    if ((fault_flags & PROT_EXEC) && !(vma->flags & PROT_EXEC)) {
        goto out_unlock;
    }
    if ((fault_flags & PROT_WRITE) && !(vma->flags & PROT_WRITE)) {
        goto out_unlock;
    }
    if ((fault_flags & PROT_READ) && !(vma->flags & PROT_READ)) {
        goto out_unlock;
    }
    if ((fault_flags & PROT_USER) && !(vma->flags & PROT_USER)) {
        goto out_unlock;
    }

    page_va = ALIGN_DOWN(fault_addr, PAGE_SIZE);
    if (pgtbl_lookup(mm->pgdir, page_va) != 0) {
        ret = 0;
        goto out_unlock;
    }

    kva = page_alloc(1);
    if (kva == NULL) {
        goto out_unlock;
    }
    memset(kva, 0, PAGE_SIZE);

    if (map(mm->pgdir, page_va, KERNEL_PA(kva), PAGE_SIZE, vma->flags) < 0) {
        kfree(kva);
        goto out_unlock;
    }

    ret = 0;

out_unlock:
    spin_unlock_irqrestore(&mm->lock, flags);
    return ret;
}
