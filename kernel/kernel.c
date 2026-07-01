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
#include <os/of.h>
#include <os/of_cpu.h>
#include <os/of_platform.h>
#include <fs/fs.h>
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

// 从设备树的 /chosen 节点获取根设备路径，如果没有找到则panic
static const char *kernel_root_device(void) {
    struct device_node *chosen;
    const char *rootdev;

    chosen = of_find_node_by_path("/chosen");
    if (chosen) {
        rootdev = of_get_property(chosen, "zzz,root-device", NULL);
        if (rootdev && rootdev[0] != '\0') {
            return rootdev;
        }
    }

    panic("No root device specified\n");
    return NULL; // 这行不会被执行，只是为了避免编译器警告
}

static const char *kernel_stdio_device(void) {
    struct device_node *chosen;
    const char *stdio_dev;

    chosen = of_find_node_by_path("/chosen");
    if (chosen) {
        stdio_dev = of_get_property(chosen, "zzz,tty-device", NULL);
        if (stdio_dev && stdio_dev[0] != '\0') {
            return stdio_dev;
        }
    }

    panic("No stdio device specified\n");
    return NULL; // 这行不会被执行，只是为了避免编译器警告
}

int kernel_init(void *arg) {
    arch_initcalls_run();
    core_initcalls_run();
    of_platform_populate(NULL,of_default_bus_match_table,NULL);
    subsys_initcalls_run();
    fs_initcalls_run();
    device_initcalls_run();
    
    mount_root(kernel_root_device(), "ext2");
    
    late_initcalls_run();
    
    setup_stdio(kernel_stdio_device());

    char *argv[] = { "/bin/init", NULL };
    
    do_execve("/bin/init", argv, NULL);
    return 0;
}

u8 is_init = 0;
unsigned long cpu_online_map = 0;

int smp_get_cpu_count(void) {
    int cpu_num = of_get_cpu_num();

    if (cpu_num <= 0) {
        return 1;
    }

    return cpu_num;
}


/* 辅助核入口：等待CPU0释放后进入idle循环 */
void secondary_entry(int cpuid) {
    int cpu = get_cpuid();
    arch_secondary_init();

    set_cpu_online(cpu);
    this_rq()->curr = this_rq()->idle;
    this_rq()->idle->on_cpu = 1;

    dprintk("CPU %d online\n", cpu);

    local_irq_enable();

    while (1) {
        cpu_idle();
        if ((current && current->need_resched) || this_rq()->nr_running > 0) {
            sched();
        }
    }
}

void welcome(){
    printk("\n\n");
    printk("\t============================================\n");
    printk("\t             Welcome to ZZZ-OS!\n");
    printk("\t============================================\n");
}

void start_kernel(int cpuid, void *dtb) {
    local_irq_disable();
    if (cpuid == 0) {
        symbols_init();
        welcome();

        early_malloc_init();

        fdt_init(dtb);
        set_cpu_online(0);
        
        memblock_init();
        initial_mm_init();
        kmalloc_init();
    
        irq_init();
        time_init();
        
        sched_init();
       
        printk("cpu %d starting\n", cpuid);
        
        arch_smp_init();

        kernel_thread(kernel_init, "kernel_init");
        pid_t pid = kernel_thread(kthreadd, "kthreadd");
        kthreadd_task = find_task_by_pid(pid);

        while(1) {
            sched();
            cpu_idle();
        }
        is_init = 1;
    } else {
        secondary_entry(cpuid);
    }
}
