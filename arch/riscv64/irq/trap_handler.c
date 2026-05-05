#include <asm/clint.h>
#include <asm/interrupt.h>
#include <asm/irq.h>
#include <asm/riscv.h>
#include <os/mm.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/syscall.h>
#include <os/types.h>
#include <mm/page_fault.h>

extern void kernel_trap_entry(void);

void trap_init(void)
{
    stvec_w((reg_t)kernel_trap_entry);
}

reg_t trap_handler(reg_t _ctx)
{
    struct pt_regs *ctx = (struct pt_regs *)_ctx;
    reg_t return_epc = ctx->sepc;
    u64 cause_code = ctx->scause & MCAUSE_MASK_CAUSECODE;
    u64 is_interrupt = ctx->scause & MCAUSE_MASK_INTERRUPT;

    if (is_interrupt) {
        switch (cause_code) {
        case CLINT_IRQ_SOFT:
        case CLINT_IRQ_TIMER:
            if (riscv64_local_irq_dispatch(0, cause_code) < 0) {
                panic("Unhandled local interrupt\n");
            }
            break;
        case CLINT_IRQ_EXTERN:
            if (handle_arch_irq != NULL) {
                handle_arch_irq((reg_t *)ctx);
            } else {
                panic("No external IRQ handler registered\n");
            }
            break;
        default:
            panic("Unknown interrupt cause\n");
        }
        return return_epc;
    }

    switch (cause_code) {
    case 0:
        panic("Instruction address misaligned!\n");
        break;
    case 1:
        panic("Instruction access fault!\n");
        break;
    case 2:
        panic("Illegal instruction !\n");
        break;
    case 3:
        panic("Breakpoint!\n");
        break;
    case 4:
        panic("Load address misaligned\n");
        break;
    case 5:
        panic("Load access fault\n");
        break;
    case 6:
        panic("Store/AMO address misaligned\n");
        break;
    case 7:
        panic("Store/AMO access fault\n");
        break;
    case 8:
        do_syscall(ctx);
        return_epc += 4;
        break;
    case 9:
        panic("Environment call from S-mode\n");
        break;
    case 11:
        panic("Environment call from M-mode\n");
        break;
    case 12:
        if (this_rq()->curr != NULL && this_rq()->curr->mm != NULL &&
            do_page_fault(this_rq()->curr->mm, stval_r(), PROT_USER | PROT_EXEC) == 0) {
            break;
        }
        panic("Instruction page fault\n");
        break;
    case 13:
        if (this_rq()->curr != NULL && this_rq()->curr->mm != NULL &&
            do_page_fault(this_rq()->curr->mm, stval_r(), PROT_USER | PROT_READ) == 0) {
            break;
        }
        panic("Load page fault\n");
        break;
    case 15:
        if (this_rq()->curr != NULL && this_rq()->curr->mm != NULL &&
            do_page_fault(this_rq()->curr->mm, stval_r(), PROT_USER | PROT_READ | PROT_WRITE) == 0) {
            break;
        }
        panic("Store/AMO page fault\n");
        break;
    default:
        panic("unknown sync exception!\ntrap!\n");
        break;
    }

    return return_epc;
}
