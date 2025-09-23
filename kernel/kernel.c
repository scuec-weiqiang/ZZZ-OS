/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-23 21:23:23
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

void set_hart_stack()
{
    char *hart_stack = (char*)page_alloc(1);
    memset(hart_stack,0,PAGE_SIZE);
    asm volatile("csrw sscratch,%0"::"r"(hart_stack + PAGE_SIZE));
}

void  init_kernel()
{   
    enum hart_id hart = HART_0;
    if(hart == HART_0) // hart0 初始化全局资源
    {
        s_global_interrupt_disable();
        zero_bss();
        symbols_init();
        trap_init();

        malloc_init();
        kernel_page_table_init();

        virt_disk_init(); 
        fs_init();

        printk("now time:%x\n",get_current_unix_timestamp(UTC8));
        is_init = 1;
    }

    set_hart_stack();
    systimer_init(hart,SYS_HZ_1);
    s_global_interrupt_enable(); 
    
    proc_init();
    struct proc* init_proc = proc_create("/user.elf");
    proc_run(init_proc);

    while(1)
    {
        // printk("tick:%d\n", systick(0));
        // printk("tick:%d\n", time_r());
        
    }
 }