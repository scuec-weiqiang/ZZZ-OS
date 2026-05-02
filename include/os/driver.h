/**
 * @FilePath     : /ZZZ-OS/include/os/driver.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-22 22:59:50
 * @LastEditTime : 2026-03-23 00:25:17
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_DRIVER_H
#define __KERNEL_DRIVER_H

#include <os/module.h>

#define driver_init() device_initcalls_run()

#endif
