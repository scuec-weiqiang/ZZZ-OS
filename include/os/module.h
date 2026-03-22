/**
 * @FilePath: /ZZZ-OS/include/os/module.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-11 20:15:50
 * @LastEditTime: 2025-11-13 23:46:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_MODULE_H__
#define __KERNEL_MODULE_H__

// #include "os/printk.h"
#include <os/compiler_attributes.h>


typedef int(*initcall_t)(void);

#define module_init(fn,section) \
    static int fn##_initcall(void) { return fn(); } \
    __section(section) __used\
    static initcall_t __initcall_##fn = fn##_initcall;


#define module_exit(fn,section) \
    static int fn##_exitcall(void) { fn(); return 0;} \
    __section(section) __used\
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
    module_init(__driver##_init,".initcall"); \
    module_exit(__driver##_exit,".exitcall");

    
#define module_irq(__driver, __register, __unregister) \
    static int  __driver##_init(void) \
    { \
        return __register(&(__driver)); \
    } \
    static void  __driver##_exit(void) \
    { \
        __unregister(&(__driver)); \
    } \
    module_init(__driver##_init,".irqinitcall"); \
    module_exit(__driver##_exit,".irqexitcall");

#define do_initcalls(start, end, type) \
    for ((type *)fn = (type *)start; fn < (type *)end; fn++) { \
        if ((*fn)() != 0) {\
            printk("Module initcall failed\n");\
        }\
    }\

#define do_exitcalls(start, end, type) \
    for ((type *)fn = (type *)start; fn < (type *)end; fn++) { \
        if ((*fn)() != 0) {\
            printk("Module exitcall failed\n");\
        }\
    }\

#endif