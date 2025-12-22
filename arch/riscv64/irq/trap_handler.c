#include <asm/arch_timer.h>
#include <asm/interrupt.h>
#include <asm/riscv.h>
#include <os/irq.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/irqreturn.h>
#include <os/mm.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/syscall.h>
#include <os/types.h>


extern void kernel_trap_entry();

void trap_init() {
    // 设置中断向量
    stvec_w((reg_t)kernel_trap_entry);
}

// 1
irqreturn_t s_soft_interrupt_handler(int virq, void *dev_id) {
    sip_w(sip_r() & ~SIP_SSIP);
    return IRQ_HANDLED;
}

// 5
irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id) {
    arch_timer_reload();
    systick_up();
    if (scheduler[tp_r()].current->expire_time >= systick()) {
        printk("\n");
        yield();
    }
    // printk("tick:%du\n",systick());
    return IRQ_HANDLED;
}

reg_t trap_handler(reg_t _ctx) {
    // printk("trap handler enter\n");
    struct trap_frame *ctx = (struct trap_frame *)_ctx;
    reg_t return_epc = ctx->sepc;
    uint64_t cause_code = ctx->scause & MCAUSE_MASK_CAUSECODE;
    uint64_t is_interrupt = (ctx->scause & MCAUSE_MASK_INTERRUPT);
    if (is_interrupt) {
        struct irq_chip *chip = irq_chip_lookup("riscv64_clint", tp_r());
        if (chip) {
            int virq = irq_domain_get_virq(chip, cause_code);
            if (virq >= 0) {
                do_irq(_ctx, (void *)(uintptr_t)virq);
            } else {
                printk("Invalid virq!\n");
            }
        } else {
            printk("IRQ chip not found!\n");
        }
    } else {
        printk("\nstval is %xu\n", stval_r());
        printk("occour in %xu\n", ctx->sepc);
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
            printk("Breakpiont!\n");
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
            // printk("Environment call from U-mode\n");
            do_syscall((struct trap_frame *)ctx);
            return_epc += 4;
            break;
        case 9:
            // printk("Environment call from S-mode\n");
            panic("Environment call from S-mode\n");
            break;
        case 11:
            // printk("Environment call from M-mode");
            panic("Environment call from M-mode\n");
            break;
        case 12:
            panic("Instruction page fault\n");
            break;
        case 13:
            panic("Load page fault\n");
            // page_fault_handler(stval_r());
            break;
        case 15:
            panic("Store/AMO page fault\n");
            // page_fault_handler(stval_r());
            break;
        default:
            panic("unknown sync exception!\ntrap!\n");
            break;
        }
    }

    return return_epc;
}

