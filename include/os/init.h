/**
 * @FilePath: /ZZZ-OS/include/os/module.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-11 20:15:50
 * @LastEditTime: 2025-11-13 23:46:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_INIT_H__
#define __KERNEL_INIT_H__ 

// #include "os/printk.h"
#include <os/compiler.h>
#include <os/printk.h>

typedef int(*initcall_t)(void);
typedef void(*exitcall_t)(void);

#define module_init(fn,section) \
    static int fn##_initcall(void) { return fn(); } \
    __section(section) __used\
    static initcall_t __initcall_##fn = fn##_initcall;

#define arch_initcall(fn) module_init(fn, ".archinitcall")
#define core_initcall(fn) module_init(fn, ".coreinitcall")
#define fs_initcall(fn) module_init(fn, ".fsinitcall")
#define device_initcall(fn) module_init(fn, ".deviceinitcall")
#define driver_initcall(fn) device_initcall(fn)
#define late_initcall(fn) module_init(fn, ".lateinitcall")

#define module_exit(fn,section) \
    static int fn##_exitcall(void) { fn(); return 0;} \
    __section(section) __used\
    static initcall_t __exitcall_##fn = fn##_exitcall;

#define do_calls(start, end, type) \
    for (type *fn = (type *)start; fn < (type *)end; fn++) { \
        if ((*fn)() != 0) {\
            printk(#type": do call failed\n");\
        }\
    }


#define do_initcalls(start, end) do_calls(start, end, initcall_t)

#define do_exitcalls(start, end) do_calls(start, end, exitcall_t)\

void arch_initcalls_run(void);
void core_initcalls_run(void);
void fs_initcalls_run(void);
void device_initcalls_run(void);
void late_initcalls_run(void);

   
#endif
