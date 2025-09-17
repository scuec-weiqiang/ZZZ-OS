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

// #include "sched.h"
// #include "systimer.h"
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

uint8_t is_init = 0;

void init_kernel()
{  
    hart_id_t hart_id = 0;
    if(hart_id == HART_0) // hart0 初始化全局资源
    {
        page_get_remain_mem();
        
        extern_interrupt_setting(hart_id,UART0_IRQN,1);
        
        virt_disk_init(); 
        vfs_init();

        // proc_init();
        // proc_t* init_proc = proc_create("/user.elf");
        // proc_run(init_proc);

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