/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-10-23 14:23:08
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
#include "fs_init.h"
#include "elf.h"
#include "proc.h"
#include "platform.h"
#include "systimer.h"
#include "symbols.h"
#include "trap_handler.h"
#include "sched.h"
#include "string.h"
#include "fdt.h"

u8 is_init = 0;

void set_hart_stack()
{
    char *hart_stack = (char*)page_alloc(1);
    memset(hart_stack,0,PAGE_SIZE);
    asm volatile("csrw sscratch,%0"::"r"(hart_stack + PAGE_SIZE));
}

void  init_kernel()
{   
    s_global_interrupt_disable();
    enum hart_id hart = HART_0;
    if(hart == HART_0) // hart0 初始化全局资源
    {
        zero_bss();
        symbols_init();
        trap_init();

        malloc_init();
        kernel_page_table_init();

        virt_disk_init(); 
        fs_init();
        timestamp_init();

        struct file *dtb = open("/qemu_virt.dtb",0);
        read(dtb,(void*)dtb_start,dtb_size);
        close(dtb);

        fdt_init((void*)dtb_start);
        fdt_test();

        struct system_time t;
        
        read(open("/time",0),(char*)&t,sizeof(t));
        printk("Current time: %d-%d-%d %d:%d:%d.%d\n", t.year, t.month, t.day, t.hour, t.minute, t.second, t.usec);
        printk("Current time:%x\n",get_current_unix_timestamp(UTC8));
        is_init = 1;
    }

    set_hart_stack();
    systimer_init(SYS_HZ_1);
    // s_global_interrupt_enable(); 

    // creat("/a.txt",S_IFREG|0644);
    // mkdir("/dir1",S_IFDIR|0644);
    
    // proc_init();
 
    // proc_create("/proc2.elf");
    // proc_create("/proc1.elf");

    while(1)
    {

    }
    sched_init(hart);
    sched();
}