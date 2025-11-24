/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-11-24 22:34:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "os/pfn.h"
#include <os/malloc.h>
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
#include <os/page.h>
#include <os/proc.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/mm/physmem.h>
#include <drivers/virtio.h>

uint8_t is_init = 0;

void set_hart_stack() {
    char *hart_stack = (char *)page_alloc(1);
    memset(hart_stack, 0, PAGE_SIZE);
    asm volatile("csrw sscratch,%0" ::"r"(hart_stack + PAGE_SIZE));
}

void dump_mapping(uint64_t satp, uintptr_t va)
{
    uint64_t root_ppn = satp & ((1ULL<<44)-1);
    uintptr_t root_pa = (uintptr_t)(root_ppn << 12);
    printk("dump mapping for va=%xu root_pa=%xu\n", (void*)va, root_pa);

    uintptr_t *table = (uintptr_t *)root_pa;
    int vpn2 = (va >> 30) & 0x1ff;
    int vpn1 = (va >> 21) & 0x1ff;
    int vpn0 = (va >> 12) & 0x1ff;
    int indices[3] = { vpn2, vpn1, vpn0 };

    for (int level = 2; level >= 0; level--) {
        uintptr_t pte = table[ indices[2-level] ];
        printk(" level %d index %d: pte=%xu\n", level, indices[2-level], pte);
        if (!(pte & 0x1)) { // V bit
            printk("  -> not present at level %d\n", level);
            return;
        }
        int r = (pte >> 1) & 1;
        int w = (pte >> 2) & 1;
        int x = (pte >> 3) & 1;
        int a = (pte >> 6) & 1;
        int d = (pte >> 7) & 1;
        printk("   flags: R=%d W=%d X=%d A=%d D=%d\n", r, w, x, a, d);
        // leaf if any of R/W/X bits set
        if (r || w || x) {
            uintptr_t ppn = (pte >> 10) & ((1ULL<<44)-1);
            uintptr_t pa = (ppn << 12) | (va & 0xfff);
            printk("   -> leaf maps va %xu -> pa %xu\n", va, pa);
            return;
        }
        // not leaf: follow next-level table
        uintptr_t next_ppn = (pte >> 10) & ((1ULL<<44)-1);
        uintptr_t next_pa = next_ppn << 12;
        table = (uintptr_t *)next_pa;
    }
}

void init_kernel(void *dtb) {
    int hart = 0;
    if (hart == 0) {
        symbols_init();
        // zero_bss();
		early_malloc_init();

		fdt_init(dtb);
        memblock_init();
        physmem_init();

		printk("a\n");
        kernel_page_table_init();
        // malloc_init();
        // of_test();
        of_platform_populate();
        irq_init();

        // printk("satp = %xu\n", satp_r());
        virt_disk_init();
        dump_mapping(satp_r(), (uintptr_t)virtio);
        fs_init();

        mkdir("/dir1",S_IFDIR|0644);
        printk("ok\n");

        do_initcalls();
        irq_enable(EXTERN_IRQ);

        is_init = 1;
    }
    printk("init success\n");
    set_hart_stack();
    arch_timer_init(SYS_HZ_1);
    // arch_timer_start();
    irq_enable(GLOBAL_IRQ);
    // memblock_reserve(KERNEL_PA(kernel_start), kernel_size);

    // creat("/a.txt",S_IFREG|0644);
    // mkdir("/dir1",S_IFDIR|0644);

    // proc_init();

    // proc_create("/proc2.elf");
    // proc_create("/proc1.elf");
    while (1) {
    }
    sched_init(hart);
    sched();
}