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

/**
 * @brief 将BSS段中的所有数据清零
 *
 * 遍历BSS段的起始地址到结束地址之间的所有字节，并将它们置为零。
 *
 * BSS段通常用于存储未初始化的全局变量和静态变量，它们在程序启动时不会自动初始化为零。
 * 本函数通过手动遍历并清零这些变量，确保它们在程序启动时是干净的。
 */
void zero_bss() {
    for (char *p = _bss_start; p < _bss_end; p++) {
        *p = 0;
    }
}

// 主核心初始化完成后，唤醒其他核心
void wakeup_other_harts() {
    // 跳过主核心（hart 0）
    for (hart_id_t hart = 1; hart < MAX_HARTS_NUM; hart++) 
    {
        // 向其他核心发送软件中断，触发其从wfi唤醒
        clint_send_ipi(hart);
    }
}


void init_kernel(hart_id_t hart_id)
{  
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        zero_bss();
        uart_init();
        page_init();
        extern_interrupt_enable();
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        task_init();
        // is_init = 1;
    }

    // while (is_init == 0){}
    wakeup_other_harts();
    
    //每个核心初始化自己的资源
    systimer_init(hart_id,SYS_HZ_100);
    trap_init();
    sched_init(hart_id);
    global_interrupt_enable();
    MACHINE_TO_USER(os_main);
}