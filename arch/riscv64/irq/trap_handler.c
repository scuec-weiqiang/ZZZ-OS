#include <drivers/uart.h>
#include <os/printk.h>
#include <asm/riscv.h>
#include <asm/arch_timer.h>
#include <asm/plic.h>
#include <drivers/uart.h>
#include <asm/clint.h>
#include <os/mm.h>
#include <os/types.h>
#include <os/syscall.h>
#include <os/sched.h>
#include <os/irq.h>
#include <asm/interrupt.h>
#include <arch/irq.h>

extern void kernel_trap_entry();

//1
reg_t s_soft_interrupt_handler(reg_t epc)
{
    sip_w(sip_r() & ~SIP_SSIP);
    
    return epc;
    
}

//3
reg_t m_soft_interrupt_handler(reg_t epc)
{
    
    sip_w(sip_r() | SIP_SSIP);
    return epc;
}

//5
reg_t s_timer_interrupt_handler(reg_t epc)
{
    enum hart_id hart_id = tp_r();
    arch_timer_reload(hart_id);
    systick_up(hart_id);
    if(scheduler[tp_r()].current->expire_time >= systick())
    {
        printk("\n");
        yield();
    }
    // printk("tick:%du\n",systick(hart_id));
    return epc;
}

//7
reg_t m_timer_interrupt_handler(reg_t epc)
{
    return epc;
}

//9
reg_t s_extern_interrupt_handler(reg_t epc)
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
    return epc;
}

//11
reg_t m_extern_interrupt_handler(reg_t epc)
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
    return epc;
}

typedef reg_t (*trap_func_t)(reg_t epc);

trap_func_t interrupt_handlers[] = {
    NULL,
    s_soft_interrupt_handler,
    NULL,
    m_soft_interrupt_handler,
    NULL,
    s_timer_interrupt_handler,
    NULL,
    m_timer_interrupt_handler,
    NULL,
    s_extern_interrupt_handler,
    NULL,
    m_extern_interrupt_handler
};

reg_t trap_handler(reg_t _ctx) {
    struct trap_frame* ctx = (struct trap_frame *)_ctx;
    reg_t return_epc = ctx->sepc;
    uint64_t cause_code = ctx->scause & MCAUSE_MASK_CAUSECODE;
    uint64_t is_interrupt = (ctx->scause & MCAUSE_MASK_INTERRUPT);
    if(is_interrupt) // 中断
    {   
        if(interrupt_handlers[cause_code] != NULL && cause_code < 12) //前12个是标准中断
        {
            return_epc = interrupt_handlers[cause_code](ctx->sepc);
        }    
        else
        {
            printk("unknown interruption!\n cause code is %l\n",cause_code);
        }    
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
    struct irq_desc clint_soft_desc = {
        .name = "clint_soft",
        .type = IRQ_TYPE_CORE,
        .irq = IRQN_SOFT,
        .hwirq = CLINT_IRQ_SOFT,
        .chip = core_irq,
        .handle = s_soft_interrupt_handler,
    };
    irq_register(&clint_soft_desc);

    struct irq_desc clint_timer_desc = {
        .name = "clint_timer",
        .type = IRQ_TYPE_CORE,
        .irq = IRQN_TIMER,
        .hwirq = CLINT_IRQ_TIMER,
        .chip = core_irq,
        .handle = s_timer_interrupt_handler,
    };
    irq_register(&clint_timer_desc);

    struct irq_desc plic_extern_desc = {
        .name = "plic_extern",
        .type = IRQ_TYPE_EXTERN,
        .irq = IRQN_EXT,
        .hwirq = CLINT_IRQ_EXTERN,
        .chip = extern_irq,
        .handle = s_extern_interrupt_handler,
    };
    irq_register(&plic_extern_desc);
    
}