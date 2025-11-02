/**
 * @FilePath: /ZZZ-OS/kernel/kernel.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-10-31 00:06:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <os/malloc.h>
#include <os/printk.h>

#include <drivers/time.h>
#include <drivers/virt_disk.h>
#include <os/mm.h>

#include <asm/clint.h>
#include <asm/plic.h>
#include <asm/riscv.h>

#include <os/elf.h>
#include <drivers/of/fdt.h>
#include <fs/fs_init.h>
#include <asm/interrupt.h>
#include <asm/platform.h>
#include <os/proc.h>
#include <os/sched.h>
#include <os/string.h>
#include <asm/symbols.h>
#include <asm/arch_timer.h>
#include <asm/trap_handler.h>
#include <drivers/uart.h>
#include <fs/vfs.h>
#include <asm/pgtbl.h>
#include <os/page.h>

uint8_t is_init = 0;

void set_hart_stack() {
	char *hart_stack = (char *)page_alloc(1);
	memset(hart_stack, 0, PAGE_SIZE);
	asm volatile("csrw sscratch,%0" ::"r"(hart_stack + PAGE_SIZE));
}

void init_kernel() {
	s_global_interrupt_disable();
	enum hart_id hart = HART_0;
	if (hart == HART_0) {
		zero_bss();
		symbols_init();
		trap_init();
		uart_reg_init();

		malloc_init();
		kernel_page_table_init();

		virt_disk_init();
		fs_init();

		struct file *dtb = open("/qemu_virt.dtb", 0);
		char *buff = malloc(dtb->f_inode->i_size);
		read(dtb, buff, dtb->f_inode->i_size);
		close(dtb);

		fdt_init(buff);
		fdt_test();
		free(buff);

		timestamp_init();
		struct system_time t;
		read(open("/time", 0), (char *)&t, sizeof(t));
		printk("Current time: %d-%d-%d %d:%d:%d.%d\n", t.year, t.month, t.day, t.hour, t.minute,
		       t.second, t.usec);
		printk("Current time:%x\n", get_current_unix_timestamp(UTC8));

		uart_init();
		write(open("/uart", 0), "hello driver\n", sizeof("hello dirver\n"));

		is_init = 1;
	}

	set_hart_stack();
	arch_timer_init(SYS_HZ_1);
	// s_global_interrupt_enable();

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