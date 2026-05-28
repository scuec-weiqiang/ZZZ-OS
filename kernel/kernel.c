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
#include <os/check.h>
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
#include <os/string.h>
#include <os/kva.h>
#include <mm/vma.h>
#include <mm/pgtbl.h>
#include <mm/pgtbl_types.h>
#include <fs/binfmt.h>
#include <fs/file.h>
#include <asm/process.h>
#include <asm/ptrace.h>

int kernel_init(void *arg) {
    of_platform_populate(NULL,of_default_bus_match_table,NULL);

    arch_initcalls_run();
    core_initcalls_run();
    fs_initcalls_run();
    device_initcalls_run();
    
    // mount_root("/dev/usdhc11", "ext2");
    mount_root("/dev/usdhc11", "ext2");
    
    late_initcalls_run();
    
    setup_stdio("/dev/uart0");
    
    char *argv[] = { "/bin/init", NULL };
    
    do_execve("/bin/init", argv, NULL);

    return 0;
}

u8 is_init = 0;

void start_kernel(int cpuid,void *dtb) {
    local_irq_disable();
    if (cpuid == 0) {
        symbols_init();
		early_malloc_init();
		fdt_init(dtb);
        memblock_init();
        initial_mm_init();
        kmalloc_init();
        irq_init();
        time_init();
        sched_init();
        kernel_thread(kernel_init, "kernel_init");
        pid_t pid= kernel_thread(kthreadd, "kthreadd");
        kthreadd_task = find_task_by_pid(pid); 

        while(1) {
            sched();
            cpu_idle();
        }
        // is_init = 1;
    }

}
