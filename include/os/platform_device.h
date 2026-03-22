/**
 * @FilePath     : /ZZZ-OS/include/os/platform_device.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:08:30
 * @LastEditTime : 2026-03-23 00:08:32
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __KERNEL_PLATFORM_DEVICE_H
#define __KERNEL_PLATFORM_DEVICE_H
#include <os/types.h>

struct platform_device {
    const char *name;              // 例如 "ns16550a"
    // struct platform_device *parent;  // optional
    void *platform_data;           // optional
    struct resource *resources;
    int num_resources;
    struct device *dev;            // device core
    struct list_head link;         // 链接到全局 platform device 列表
};

#endif