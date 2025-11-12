#include <drivers/uart.h>
#include <os/printk.h>
#include <asm/riscv.h>
#include <asm/arch_timer.h>
#include <asm/riscv_plic.h>
#include <drivers/uart.h>
#include <os/mm.h>
#include <os/types.h>
#include <os/syscall.h>
#include <os/sched.h>
#include <os/irq.h>
#include <asm/interrupt.h>
#include <os/irq_chip.h>
#include <os/irq_domain.h>
#include <os/irq.h>
#include <os/irqreturn.h>

extern void kernel_trap_entry();


//1
irqreturn_t s_soft_interrupt_handler(int virq, void *dev_id)
{
    sip_w(sip_r() & ~SIP_SSIP);
    
    return IRQ_HANDLED;
    
}

//3
irqreturn_t m_soft_interrupt_handler(int virq, void *dev_id)
{
    
    sip_w(sip_r() | SIP_SSIP);
    return IRQ_HANDLED;
}

//5
irqreturn_t s_timer_interrupt_handler(int virq, void *dev_id)
{
    arch_timer_reload();
    systick_up();
    // if(scheduler[tp_r()].current->expire_time >= systick())
    // {
    //     printk("\n");
    //     yield();
    // }
    printk("tick:%du\n",systick());
    return IRQ_HANDLED;
}

//7
irqreturn_t m_timer_interrupt_handler(int virq, void *dev_id)
{
    return IRQ_HANDLED;
}

//9
irqreturn_t s_extern_interrupt_handler(int virq, void *dev_id)
{
    enum hart_id hart_id = tp_r();
    uint32_t irqn = __plic_claim(hart_id);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
        break;

        default:
            printk("unexpected extern interrupt!");
        break;
    }
    if(irqn)
    {
        __plic_complete(0,irqn);
    }
    return IRQ_HANDLED;
}

//11
irqreturn_t m_extern_interrupt_handler(int virq, void *dev_id)
{
    enum hart_id hart_id = tp_r();
    uint32_t irqn = __plic_claim(hart_id);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
        break;

        default:
            printk("unexpected extern interrupt!");
        break;
    }
    if(irqn)
    {
        __plic_complete(0,irqn);
    }
    return IRQ_HANDLED;
}

reg_t trap_handler(reg_t _ctx) {
    printk("trap handler enter\n");
    struct trap_frame* ctx = (struct trap_frame *)_ctx;
    reg_t return_epc = ctx->sepc;
    uint64_t cause_code = ctx->scause & MCAUSE_MASK_CAUSECODE;
    uint64_t is_interrupt = (ctx->scause & MCAUSE_MASK_INTERRUPT);
    if(is_interrupt) // 中断
    {   
        struct irq_chip *chip = irq_chip_lookup("riscv64_clint");
        int virq = irq_domain_get_virq(chip, cause_code);
        do_irq(_ctx, (void *)(uintptr_t)virq);
    }
    else
    {
        // printk("\nstval is %xu\n",stval_r());
        // printk("occour in %xu\n",epc);
        switch (cause_code)
        {
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
                // panic("\033[32mStore/AMO access fault\n\033[0m");
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


void trap_init()
{
    // 设置中断向量
    stvec_w((reg_t)kernel_trap_entry);
}