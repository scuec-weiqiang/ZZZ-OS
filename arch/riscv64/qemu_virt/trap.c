#include "uart.h"
#include "printf.h"
#include "riscv.h"
#include "systimer.h"
#include "swtimer.h"
#include "plic.h"
#include "uart.h"
#include "sched.h"

extern void trap_entry();

reg_t timer_interrupt_handler(reg_t epc);
void extern_interrupt_handler();

void trap_init()
{
    mtvec_w((reg_t)trap_entry);
}


/***************************************************************
 * @description: 
 * @param {uint32_t} mcause [in/out]:  
 * @param {uint32_t} mtval [in/out]:  
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t trap_handler(reg_t epc,reg_t cause,reg_t ctx)
{
    reg_t return_epc = epc;
    uint64_t cause_code = cause & MCAUSE_MASK_CAUSECODE;

    if((cause & MCAUSE_MASK_INTERRUPT))
    {
        switch (cause_code)
        {
            case 3:
                printf("software interruption!\n");
                break;
            case 7:
                // printf("timer interruption!\n");
                return_epc =  timer_interrupt_handler(epc);
                break;
            case 11:
                // printf("external interruption!\n");
                extern_interrupt_handler();
                break;
            default:
                printf("unknown async exception!\n cause code is %l\n",cause_code);
                printf("mstatus:%x,mie:%x\n",mstatus_r(),mie_r());
                break;
        }
    }
    else
    {
        // printf("\nmtval is %x\n",mtval_r());
        // printf("occour in %x\n",epc);
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
                printf("Breakpiont!\n");
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
                // printf("Environment call from U-mode\n");
                extern do_syscall(reg_context_t *ctx);
                do_syscall(ctx);
                return_epc += 4;
                break;
            case 9:
                // printf("Environment call from S-mode\n");
                panic("Environment call from S-mode\n");
                break;
            case 11:
                // printf("Environment call from M-mode");
                panic("Environment call from M-mode\n");
                break;
            case 12:
                panic("Instruction page fault\n");
                break;
            case 13:
                panic("Load page fault\n");
                break;
            case 15:
                panic("Store/AMO page fault\n");
                break;
            default:
                panic("unknown sync exception!\ntrap!\n");
                break;
        }
    }
    return return_epc;
}

/***************************************************************
 * @description: 
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
void extern_interrupt_handler()
{
    hart_id_t hart_id = mhartid_r();
    uint32_t irqn = __plic_claim(hart_id);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
        break;

        default:
            printf("unexpected extern interrupt!");
        break;
    }
    if(irqn)
    {
        __plic_complete(0,irqn);
    }
}

/***************************************************************
 * @description: 
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
reg_t timer_interrupt_handler(reg_t epc )
{
    reg_t r;
    hart_id_t hart_id = mhartid_r();
    // printf("hart %d timer interrupt!\n",hart_id);
    uint64_t now_time = systimer_get_time();
    // systimer_tick++;

    r = sched(epc,now_time,hart_id);
    
    // swtimer_check();
    systimer_load(hart_id,systimer_hz);

    return r;
}

void soft_interrupt_handler()
{
    __clint_clear_ipi(mhartid_r());
    // *(uint32_t*)CLINT_MSIP(0)=0;
    // __sw_without_save(&sched_context);
}

