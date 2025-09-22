/**
 * @FilePath: /vboot/home/wei/ZZZ/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-21 17:49:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "printk.h"
#include "malloc.h"

#include "vm.h"
#include "virt_disk.h"
#include "time.h"


#include "riscv.h"
#include "plic.h"
#include "clint.h"

#include "interrupt.h"
#include "vfs.h"
#include "elf.h"
#include "proc.h"
#include "platform.h"
#include "systimer.h"
#include "symbols.h"
#include "trap_handler.h"

u8 is_init = 0;



void  init_kernel()
{   
    enum hart_id hart = HART_0;
    if(hart == HART_0) // hart0 初始化全局资源
    {
        s_global_interrupt_disable();
        zero_bss();
        symbols_init();
        trap_init();

        // systimer_init(hart,SYS_HZ_1000);

        // // extern_interrupt_setting(hart,UART0_IRQN,1);

        malloc_init();
        kernel_page_table_init();

        virt_disk_init(); 
        fs_init();

        // proc_init();
        // struct proc* init_proc = proc_create("/user.elf");
        // proc_run(init_proc);

        printk("now time:%x\n",get_current_unix_timestamp(UTC8));
        is_init = 1;

    }

    printk("hart_id:%d\n", hart);
    while (is_init == 0){}
    // wakeup_other_harts();
 
    // s_global_interrupt_enable(); 
    //每个核心初始化自己的资源
    // systimer_init(hart_id,SYS_HZ_100);
    // sched_init(hart_id);
    // __clint_send_ipi(0);
    // sip_w(sip_r() | 2);
   
    while(1)
    {
        // printk("hart_id:%d\n", hart_id++);
    }
    // global_interrupt_enable();
    // M_TO_U(os_main);
 }