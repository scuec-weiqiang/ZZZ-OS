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
#include <os/compiler_attributes.h>
#include <os/printk.h>

typedef int(*initcall_t)(void);
typedef void(*exitcall_t)(void);

#define module_init(fn,section) \
    static int fn##_initcall(void) { return fn(); } \
    __section(section) __used\
    static initcall_t __initcall_##fn = fn##_initcall;

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

   
#endif