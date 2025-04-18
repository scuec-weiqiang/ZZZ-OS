#include "riscv.h"
#include "plic.h"
#include "maddr_def.h"
#include "interrupt.h"
#include "hwtimer.h"

#include "printf.h"
#include "page.h"
#include "uart.h"
#include "sched.h"
#include "trap.h"
extern void os_main();

void main(int hartid)
{   
    if(hartid == 0)
    {

        //给内核分配一个上下文，在没有任务执行时，内核用这个上下文运行
        mscratch_w(_kernel_reg_ctx_start);

        trap_init();
        uart_init();
        page_init();
        sched_init();

        global_interrupt_enable();

        USER_MODE_INIT;
        // MACHINE_MODE_INIT;
        // reg_t mstatus = mstatus_r();
        // mstatus &= ~(3 << 11);  // 清除MPP
        // mstatus |= (0 << 11);   // MPP=U模式
        // mstatus |= (1 << 3);    // MIE=1（启用中断）
        // mstatus_w(mstatus);

       // 清除 MPP 字段后设置为 S 模式 (01)，并启用 MPIE

        hwtimer_reload_s(1);
        timer_interrupt_enable();

        extern_interrupt_enable();
        extern_interrupt_setting(HART_0,UART0_IRQN,1);

        mepc_w((reg_t)os_main);
        asm volatile("mret");

        // printf("core 0 running\nsystem init...\n");
        

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