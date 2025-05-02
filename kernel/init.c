/**
 * @FilePath: /ZZZ/kernel/init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 00:43:47
 * @LastEditTime: 2025-05-02 19:24:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
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
uint8_t is_init = 0;

void init(hart_id_t hart_id)
{  
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        uart_init();
        page_init();
        extern_interrupt_enable();
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        task_init();
        is_init = 1;
    }

    while (is_init == 0){}
    
    //每个核心初始化自己的资源
    systimer_init(hart_id,SYS_HZ_100);
    trap_init();
    sched_init(hart_id);
    global_interrupt_enable();
    MACHINE_TO_USER(os_main);
}