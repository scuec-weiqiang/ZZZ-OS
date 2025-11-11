/**
 * @FilePath: /ZZZ-OS/include/os/module.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-11 20:15:50
 * @LastEditTime: 2025-11-11 21:16:18
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
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


#define module_exit(fn) \
    static int fn##_exitcall(void) { fn(); return 0;} \
    __section(".exitcall") __used\
    static initcall_t __exitcall_##fn = fn##_exitcall;

#define module_driver(__driver, __register, __unregister) \
    static int  __driver##_init(void) \
    { \
        return __register(&(__driver)); \
    } \
    static void  __driver##_exit(void) \
    { \
        __unregister(&(__driver)); \
    } \
    module_init(__driver##_init); \
    module_exit(__driver##_exit);


// 启动阶段执行
static __used void do_initcalls(void) {
    for (initcall_t *fn = (initcall_t *)initcall_start; fn < (initcall_t *)initcall_end; fn++) {
        if ((*fn)() != 0) {
            printk("Module initcall failed\n");
            // 处理初始化失败
        }
    }
}

static __used void do_exitcalls(void) {
    for (initcall_t *fn = (initcall_t *)exitcall_start; fn < (initcall_t *)exitcall_end; fn++) {
        if ((*fn)() != 0) {
            printk("Module exitcall failed\n");
            // 处理退出失败
        }
    }
}
#endif