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
        switch (cause_code)
        {
            case 0:
                printf("\nmtval is %x\n",mtval_r());
                printf("occour in %x\n",epc);
                panic("instruction address misaligned!\n");
                break;
            case 1:
                printf("\nmtval is %x\n",mtval_r());
                printf("occour in %x\n",epc);
                panic("instruction access fault!\n");
                break;
            case 2:
                printf("\nmtval is %x\n",mtval_r());
                printf("occour in %x\n",epc);
                panic("illegal instruction !\n");
                break;
            case 3:
                printf("\nmtval is %x\n",mtval_r());
                printf("breakpiont!\n");
                break;
            case 8:
                printf("environment call from U-mode");
                break;
            case 9:
                printf("environment call from S-mode");
                break;
            case 11:
                // printf("external interruption!\n");
                printf("environment call from M-mode");
                break;
            default:
                printf("unknown sync exception!\n cause code is %l\n",cause_code);
                printf("mtval is %x\n",mtval_r());
                panic("trap!");
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
    uint32_t irqn = __plic_claim(0);
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
    uint64_t now_time = systimer_get_time();
    systimer_tick++;

    r = sched(epc,now_time);
    
    swtimer_check();
    systimer_load(HART_0,systimer_hz);

    return r;
}

void soft_interrupt_handler()
{
    // *(uint32_t*)CLINT_MSIP(0)=0;
    // __sw_without_save(&sched_context);
}

