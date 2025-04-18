#include "uart.h"
#include "printf.h"
#include "riscv.h"
#include "hwtimer.h"
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
reg_t trap_handler(reg_t epc,reg_t cause)
{
    reg_t return_epc = epc;
    uint64_t cause_code = cause & MCAUSE_MASK_CAUSECODE;
    if((cause & MCAUSE_MASK_INTERRUPT))
    {
        switch (cause_code)
        {
            case 3:
                // printf("software interruption!\n");
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
        printf("casue code is %d\n",cause_code);
        printf("mtval is %x\n",mtval_r());
        panic("trap!\n"); 
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
    uint32_t irqn = __plic_claim(0);
    switch (irqn)
    {
        case 10:
            uart0_iqr();
            // printf("uart!\n");
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
    // hwtimer_reload_s(1);
    reg_t r;
    // swtimer_check();
    r = sched(epc);
    return r;
}

void soft_interrupt_handler()
{
    // *(uint32_t*)CLINT_MSIP(0)=0;
    // __sw_without_save(&sched_context);
}

