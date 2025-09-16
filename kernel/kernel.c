/**
 * @FilePath: /ZZZ/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-16 22:07:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printf.h"
#include "page_alloc.h"
#include "uart.h"
// #include "sched.h"
// #include "systimer.h"
#include "vm.h"
#include "virt_disk.h"
#include "time.h"


#include "riscv.h"
#include "plic.h"
#include "clint.h"
#include "maddr_def.h"
#include "interrupt.h"
#include "vfs.h"
#include "string.h"
#include "elf.h"
#include "string.h"
#include "proc.h"

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


void init_kernel()
{  
    hart_id_t hart_id = 0;
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        // int a = *(volatile uint32_t *)0xffffffffc0000000;
        zero_bss();
        uart_init();
        page_alloc_init();
        page_get_remain_mem();
        kernel_page_table_init();
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        virt_disk_init(); 

        vfs_init();

        proc_init();
        proc_t* init_proc = proc_create("/user.elf");
        proc_run(init_proc);

        printf("now time:%x\n",get_current_unix_timestamp(UTC8));
        page_get_remain_mem();
        is_init = 1;

    }

    printf("hart_id:%d\n", hart_id);
    while (is_init == 0){}
    // wakeup_other_harts();
 
    s_global_interrupt_enable(); 
    //每个核心初始化自己的资源
    // systimer_init(hart_id,SYS_HZ_100);
    // sched_init(hart_id);
    // __clint_send_ipi(0);
    // sip_w(sip_r() | 2);
   
    while(1)
    {
        // printf("hart_id:%d\n", hart_id++);
    }
    // global_interrupt_enable();
    // M_TO_U(os_main);
 }