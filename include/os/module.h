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

#include <os/init.h>


#define module_driver(__driver, __register, __unregister) \
    static int  __driver##_init(void) \
    { \
        return __register(&(__driver)); \
    } \
    static void  __driver##_exit(void) \
    { \
        __unregister(&(__driver)); \
    } \
    device_initcall(__driver##_init); \
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

#endif
