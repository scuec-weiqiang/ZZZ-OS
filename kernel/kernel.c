/**
 * @FilePath: /ZZZ/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-06-29 22:07:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printf.h"
#include "page_alloc.h"
#include "uart.h"
#include "sched.h"
#include "systimer.h"
#include "vm.h"
#include "virt_disk.h"
#include "ext2.h"

#include "riscv.h"
#include "plic.h"
#include "clint.h"
#include "maddr_def.h"
#include "interrupt.h"
#include "systimer.h"
#include "string.h"


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
        __clint_send_ipi(hart);
    }
}


void init_kernel()
{  
    trap_init();
    hart_id_t hart_id = 0;
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        zero_bss();
        uart_init();
        page_alloc_init();
        kernel_page_table_init();
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        virt_disk_init(); 
        file_system_test();
        // task_init();
        is_init = 1;

    }
    // ext2_create_dir_by_path(fs, "/a/b/");
    printf("hart_id:%d\n", hart_id);
    while (is_init == 0){}
    // wakeup_other_harts();
    
    //每个核心初始化自己的资源
    // systimer_init(hart_id,SYS_HZ_100);
    // sched_init(hart_id);
    // __clint_send_ipi(0);
    // sip_w(sip_r() | 2);
   
    while(1)
    {

    }
    global_interrupt_enable();
    M_TO_U(os_main);
 }