/**
 * @FilePath: /ZZZ-OS/include/drivers/core/driver.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-10 20:21:56
 * @LastEditTime: 2025-11-11 21:10:06
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_DRIVER_H
#define __KERNEL_DRIVER_H

#include <os/driver_model.h>
#include <os/module.h>

extern int platform_driver_register(struct platform_driver *drv);
extern int platform_driver_unregister(struct platform_driver *drv);

#define module_platform_driver(__platform_driver) \
    module_driver(__platform_driver, platform_driver_register, platform_driver_unregister)


#endif // OS_DRIVER_H