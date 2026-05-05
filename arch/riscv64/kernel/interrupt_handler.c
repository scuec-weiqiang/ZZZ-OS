#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/irq.h>
#include <asm/process.h>
#include <asm/ptrace.h>
#include <asm/riscv.h>
#include <asm/trap_handler.h>
#include <mm/page_fault.h>
#include <os/mm.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/stacktrace.h>
#include <os/syscall.h>

extern void kernel_trap_entry(void);

static void trap_panic(const char *reason, struct pt_regs *regs)
{
    printk("riscv64 trap: %s\n", reason);
    if (regs != NULL) {
        show_regs(regs);
    }
    dump_stack();
    while (1) {
    }
}

void trap_init(void)
{
    stvec_w((reg_t)kernel_trap_entry);
}

static int handle_page_fault(struct pt_regs *regs, int prot)
{
    if (!pt_regs_is_user(regs) || current == NULL || current->mm == NULL) {
        return -1;
    }

    return do_page_fault(current->mm, regs->stval, prot);
}

struct pt_regs *trap_prepare_user_return(struct pt_regs *regs)
{
    (void)regs;
    sched_handle_user_return();
    return task_pt_regs(current);
}

reg_t trap_handler(reg_t ctx)
{
    struct pt_regs *regs = (struct pt_regs *)ctx;
    reg_t cause = regs->scause;
    reg_t code = cause & MCAUSE_MASK_CAUSECODE;
    bool is_interrupt = (cause & MCAUSE_MASK_INTERRUPT) != 0;

    if (is_interrupt) {
        switch (code) {
        case CLINT_IRQ_SOFT:
        case CLINT_IRQ_TIMER:
            if (riscv64_local_irq_dispatch(ctx, code) < 0) {
                trap_panic("unhandled local interrupt", regs);
            }
            break;
        case CLINT_IRQ_EXTERN:
            if (handle_arch_irq != NULL) {
                handle_arch_irq((reg_t *)regs);
            } else {
                trap_panic("no external IRQ handler registered", regs);
            }
            break;
        default:
            trap_panic("unknown interrupt cause", regs);
        }

        irq_run_deferred_works();
        return regs->sepc;
    }

    switch (code) {
    case 2:
        trap_panic("illegal instruction", regs);
        break;
    case 3:
        trap_panic("breakpoint", regs);
        break;
    case 8:
        do_syscall(regs);
        regs->sepc += 4;
        break;
    case 12:
        if (handle_page_fault(regs, PROT_USER | PROT_EXEC) == 0) {
            break;
        }
        trap_panic("instruction page fault", regs);
        break;
    case 13:
        if (handle_page_fault(regs, PROT_USER | PROT_READ) == 0) {
            break;
        }
        trap_panic("load page fault", regs);
        break;
    case 15:
        if (handle_page_fault(regs, PROT_USER | PROT_READ | PROT_WRITE) == 0) {
            break;
        }
        trap_panic("store page fault", regs);
        break;
    default:
        trap_panic("unknown synchronous exception", regs);
        break;
    }

    return regs->sepc;
}
