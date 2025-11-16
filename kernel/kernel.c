/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-11-17 00:02:16
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <os/malloc.h>
#include <os/printk.h>

#include <drivers/time.h>
#include <drivers/virt_disk.h>
#include <os/mm.h>

#include <os/elf.h>
#include <drivers/of/fdt.h>
#include <fs/fs_init.h>
#include <os/proc.h>
#include <os/sched.h>
#include <os/string.h>
#include <asm/symbols.h>
#include <asm/arch_timer.h>
#include <fs/vfs.h>
#include <os/page.h>
#include <drivers/of/of_platform.h>
#include <os/of.h>
#include <os/module.h>
#include <os/irq.h>
#include <os/mm/memblock.h>

uint8_t is_init = 0;

void set_hart_stack() {
	char *hart_stack = (char *)page_alloc(1);
	memset(hart_stack, 0, PAGE_SIZE);
	asm volatile("csrw sscratch,%0" ::"r"(hart_stack + PAGE_SIZE));
}

void init_kernel() {
	int hart = 0;
	if (hart == 0) {
		zero_bss();
		symbols_init();
		memblock_init();
		malloc_init();

		irq_init();
		kernel_page_table_init();

		virt_disk_init();
		fs_init();

		struct file *dtb = open("/qemu_virt.dtb", 0);
		char *buff = malloc(dtb->f_inode->i_size);
		read(dtb, buff, dtb->f_inode->i_size);
		close(dtb);

		fdt_init(buff);
		of_test();
		of_platform_populate();
		free(buff);

		do_initcalls();
		irq_enable(EXTERN_IRQ);
		
		is_init = 1;
	} 
	printk("init success\n");
	set_hart_stack();
	arch_timer_init(SYS_HZ_1);
	// arch_timer_start();
	// irq_enable(GLOBAL_IRQ);
	of_scan_memory();
	of_scan_reserved_memory();
	memblock_dump();
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