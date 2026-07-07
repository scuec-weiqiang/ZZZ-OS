#include <os/sched.h>
#include <os/mm.h>
#include <os/spinlock.h>
#include <asm/syscall_num.h>
#include <fs/binfmt.h>

__SYSCALL__ long sys_brk(struct pt_regs *ctx)
{
    struct mm_struct *mm = current->mm;
    unsigned long new_brk = ctx->r[0];
    unsigned long flags;
    unsigned long ret;

    // dprintk("brk: req=%x start_brk=%x brk=%x start_stack=%x stack_limit=%x\n",
    //         (unsigned int)new_brk,
    //         (unsigned int)mm->start_brk,
    //         (unsigned int)mm->brk,
    //         (unsigned int)mm->start_stack,
    //         (unsigned int)(USER_STACK_TOP - USER_STACK_SIZE));

    if (mm == NULL) {
        return 0;
    }

    flags = spin_lock_irqsave(&mm->lock);

    if (new_brk == 0) {
        ret = mm->brk;
        goto out_unlock;
    }

    if (new_brk < mm->start_brk) {
        ret = mm->brk;
        goto out_unlock;
    }

    if (new_brk >= USER_SIGTRAMP_ADDR) {
        ret = mm->brk;
        goto out_unlock;
    }
    // dprintk("new brk = %x\n", new_brk);
    if (new_brk > mm->brk) {
        unsigned long old_page = ALIGN_UP(mm->brk, PAGE_SIZE);
        unsigned long new_page = ALIGN_UP(new_brk, PAGE_SIZE);
        // dprintk("brk: grow old_page=%x new_page=%x\n",
        //         (unsigned int)old_page, (unsigned int)new_page);
        for (unsigned long va = old_page; va < new_page; va += PAGE_SIZE) {
            // dprintk("brk: add vma page=%x\n", (unsigned int)va);
            vma_add(mm, (virt_addr_t)va, PAGE_SIZE, PROT_USER | PROT_READ | PROT_WRITE);
        }
    }
    mm->brk = new_brk;
    // dprintk("brk: new brk=%x\n", (unsigned int)mm->brk);
    ret = mm->brk;

out_unlock:
    spin_unlock_irqrestore(&mm->lock, flags);
    return ret;
}
