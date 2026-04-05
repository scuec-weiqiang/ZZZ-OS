/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-12-03 18:26:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
// #include "os/pfn.h"
#include <os/kmalloc.h>
#include <os/printk.h>

// #include <os/time.h>
#include <os/mm.h>

// #include <asm/arch_timer.h>
#include <os/fdt.h>
#include <os/of_platform.h>
// #include <fs/fs_init.h>
// #include <fs/vfs.h>
// #include <os/elf.h>
#include <os/irq.h>
#include <os/timer_chip.h>
#include <mm/memblock.h>
#include <mm/symbols.h>
#include <mm/early_malloc.h>
#include <os/device.h>
// #include <os/of.h>
// #include <mm/page.h>
// #include <os/proc.h>
// #include <os/sched.h>
// #include <os/string.h>
// #include <mm/physmem.h>
// #include <mm/buddy.h>
// #include <mm/slab.h>

uint8_t is_init = 0;

// void set_cpu_stack() {
//     char *cpu_stack = (char *)page_alloc(1);
//     memset(cpu_stack, 0, PAGE_SIZE);
//     asm volatile("csrw sscratch,%0" ::"r"(cpu_stack + PAGE_SIZE));
// }

void init_kernel(int cpuid,void *dtb) {
    if (cpuid == 0) {
        symbols_init();
		early_malloc_init();

        printk("kernel init start\n");
        printk("dtb address = %xu\n", (uintptr_t)dtb);
        
		fdt_init(dtb);
        memblock_init();
        mm_init();
    
        kmalloc_init();

        of_platform_populate(NULL,of_default_bus_match_table,NULL);

        irq_init();
        timer_chip_init();
        driver_init();

        // fs_init();
        // irq_enable(EXTERN_IRQ);

        // is_init = 1;
    }



    printk("kernel init end\n");

    // set_cpu_stack();
    // arch_timer_init(SYS_HZ_1);
    // irq_enable(GLOBAL_IRQ);

    // proc_init();

    // proc_create("/proc2.elf");
    // proc_create("/proc1.elf");

    // sched_init(cpuid);
    // sched();

    while(1);
}
