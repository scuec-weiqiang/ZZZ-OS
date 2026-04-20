/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-12-03 18:26:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */

#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/fdt.h>
#include <os/of_platform.h>
#include <fs/fs.h>
// #include <os/elf.h>
#include <os/irq.h>
#include <os/timer_chip.h>
#include <os/timekeeping.h>
#include <mm/memblock.h>
#include <mm/symbols.h>
#include <mm/early_malloc.h>
#include <os/device.h>
#include <os/timerqueue.h>
#include <os/cpu.h>
#include <os/completion.h>

int kernel_init(void *arg) {
    printk("kernel init \n");
    of_platform_populate(NULL,of_default_bus_match_table,NULL);
    driver_init();
    fs2_init();
    sched_kthread_test(); // 创建两个测试进程
    return 0;
}

uint8_t is_init = 0;

void start_kernel(int cpuid,void *dtb) {
    local_irq_disable();
    if (cpuid == 0) {
        printk("kernel init start\n");
        symbols_init();
		early_malloc_init();
		fdt_init(dtb);
        memblock_init();
        initial_mm_init();
        kmalloc_init();
        irq_init();
        
        time_init();
        sched_init();
        kernel_thread(kernel_init, "kernel_init", CLONE_FS);
        pid_t pid= kernel_thread(kthreadd, "kthreadd", CLONE_FS);
        kthreadd_task = find_task_by_pid(pid); 
        
       
        while(1) {
            sched();
            cpu_idle();
        }
        // is_init = 1;
    }

}
