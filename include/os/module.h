#ifndef __KERNEL_MODULE_H__
#define __KERNEL_MODULE_H__

#include "os/printk.h"
#include <os/compiler_attributes.h>
#include <asm/symbols.h>

typedef int(*initcall_t)(void);

#define module_init(fn) \
    static int fn##_initcall(void) { return fn(); } \
    __section(".initcall") __used\
    static initcall_t __initcall_##fn = fn##_initcall;

// 启动阶段执行
static __used void do_initcalls(void) {
    for (initcall_t *fn = (initcall_t *)initcall_start; fn < (initcall_t *)initcall_end; fn++) {
        if ((*fn)() != 0) {
            printk("Module initcall failed\n");
            // 处理初始化失败
        }
    }
}

#endif