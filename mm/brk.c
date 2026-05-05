#include <os/sched.h>
#include <os/mm.h>
#include <os/syscall_num.h>
#include <fs/binfmt.h>

long sys_brk(struct pt_regs *ctx)
{
    struct mm_struct *mm = current->mm;
    unsigned long new_brk = ctx->r[0];

    // dprintk("brk: req=%x start_brk=%x brk=%x start_stack=%x stack_limit=%x\n",
    //         (unsigned int)new_brk,
    //         (unsigned int)mm->start_brk,
    //         (unsigned int)mm->brk,
    //         (unsigned int)mm->start_stack,
    //         (unsigned int)(USER_STACK_TOP - USER_STACK_SIZE));

    if (new_brk == 0)
        return mm->brk;

    if (new_brk < mm->start_brk)
        return mm->brk;

    if (new_brk >= USER_STACK_TOP - USER_STACK_SIZE)
        return mm->brk;
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
    return mm->brk;
}
