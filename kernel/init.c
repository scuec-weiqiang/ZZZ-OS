#include "riscv.h"
#include "plic.h"
#include "maddr_def.h"
#include "interrupt.h"
#include "systimer.h"

#include "printf.h"
#include "page.h"
#include "uart.h"
#include "sched.h"
#include "trap.h"
#include "systimer.h"

extern void os_main();

void main(int hartid)
{   
    if(hartid == 0)
    {
        systimer_init(SYS_HZ_100);

        trap_init();
        uart_init();
        page_init();
        sched_init();
        
        extern_interrupt_enable();
        extern_interrupt_setting(HART_0,UART0_IRQN,1);

        global_interrupt_enable();

        printf("core 0 running\nsystem init...\n");

        MACHINE_TO_USER(os_main);

    }
    else if(hartid == 1)
    {

    }
    else
    {

    }
    uint64_t a;
    while(1)
    {
        a++;
        // *((volatile uint8_t*)a ) = 0;
    }
}