#include "uart.h"
#include "printf.h"


/***************************************************************
 * @description: 
 * @param {uint32_t} mcause [in/out]:  
 * @param {uint32_t} mtval [in/out]:  
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
void trap_handler(uint32_t mcause,uint32_t mtval,uint32_t mepc)
{
    uint32_t code = mcause & 0x7fffffff;
   
    if(!(mcause & 0x80000000))//异常
    {
        printf("casue code is %d\n",code);
        printf("mtval is %d\n",mtval);
        panic("trap!\n"); 
    }
}

/***************************************************************
 * @description: 
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
uint32_t extern_interrupt_handler(uint32_t mepc)
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
    __plic_complete(0,irqn);
    return mepc+4;

}

/***************************************************************
 * @description: 
 * @param {uint32_t} mepc [in/out]:  
 * @return {*}
***************************************************************/
uint32_t timer_interrupt_handler(uint32_t mepc)
{
    // printf("timer interrupt:%x\n",mstatus_r());
    hwtimer_tick++;
    hwtimer_ms(1);
    swtimer_check();

    sched();
    
    return mepc+4;
}

void soft_interrupt_handler()
{
    *(uint32_t*)CLINT_MSIP(0)=0;
    __sw_without_save(&sched_context);
}

