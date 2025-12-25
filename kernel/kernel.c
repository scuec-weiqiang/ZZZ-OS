/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-12-03 18:26:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "os/pfn.h"
#include <os/kmalloc.h>
#include <os/printk.h>

#include <drivers/time.h>
#include <drivers/virt_disk.h>
#include <os/mm.h>

#include <asm/arch_timer.h>
#include <drivers/of/fdt.h>
#include <drivers/of/of_platform.h>
#include <fs/fs_init.h>
#include <fs/vfs.h>
#include <os/elf.h>
#include <os/irq.h>
#include <os/mm/memblock.h>
#include <os/mm/symbols.h>
#include <os/mm/early_malloc.h>
#include <os/module.h>
#include <os/of.h>
#include <os/mm/page.h>
#include <os/proc.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/mm/physmem.h>
#include <drivers/virtio.h>
#include <os/mm/buddy.h>
#include <os/mm/slab.h>

uint8_t is_init = 0;

void set_hart_stack() {
    char *hart_stack = (char *)page_alloc(1);
    memset(hart_stack, 0, PAGE_SIZE);
    asm volatile("csrw sscratch,%0" ::"r"(hart_stack + PAGE_SIZE));
}

void init_kernel(void *dtb) {
    int hart = 0;
    if (hart == 0) {
        symbols_init();
        // zero_bss();
		early_malloc_init();
		fdt_init(dtb);
        memblock_init();
        mm_init();
        kmalloc_init();
        of_platform_populate();
        irq_init();

        virt_disk_init();
        fs_init();

        do_initcalls();
        irq_enable(EXTERN_IRQ);

        is_init = 1;
    }
    set_hart_stack();
    arch_timer_init(SYS_HZ_1);
    irq_enable(GLOBAL_IRQ);

    proc_init();

    proc_create("/proc2.elf");
    proc_create("/proc1.elf");

    sched_init(hart);
    sched();
}