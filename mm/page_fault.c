#include <mm/vma.h>
#include <mm/mm_types.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/pfn.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/string.h>
#include <os/err.h>
#include <mm/pgtbl.h>

int do_page_fault(struct mm_struct *mm, virt_addr_t fault_addr, int fault_flags) {
    virt_addr_t page_va;
    struct vma *vma;
    void *kva;
    here;
    if (!mm || mm->pgdir == NULL) {
        return -1;
    }

    vma = vma_find(mm, fault_addr);
    if (IS_ERR(vma)) {
        return -1;
    }

    if ((fault_flags & PROT_EXEC) && !(vma->flags & PROT_EXEC)) {
        return -1;
    }
    if ((fault_flags & PROT_WRITE) && !(vma->flags & PROT_WRITE)) {
        return -1;
    }
    if ((fault_flags & PROT_READ) && !(vma->flags & PROT_READ)) {
        return -1;
    }
    if ((fault_flags & PROT_USER) && !(vma->flags & PROT_USER)) {
        return -1;
    }

    page_va = ALIGN_DOWN(fault_addr, PAGE_SIZE);
    if (pgtbl_lookup(mm->pgdir, page_va) != 0) {
        return 0;
    }

    kva = page_alloc(1);
    if (kva == NULL) {
        return -1;
    }
    memset(kva, 0, PAGE_SIZE);

    if (map(mm->pgdir, page_va, KERNEL_PA(kva), PAGE_SIZE, vma->flags) < 0) {
        return -1;
    }

    return 0;
}
