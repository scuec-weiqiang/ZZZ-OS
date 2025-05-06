/*******************************************************************************************
 * @FilePath: /ZZZ/kernel/init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 00:43:47
 * @LastEditTime: 2025-04-20 15:07:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
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

void init(int hartid)
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
    while(1)
    {
    }
}