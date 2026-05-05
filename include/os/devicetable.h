/**
 * @FilePath     : /ZZZ-OS/include/os/devicetable.h
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-23 00:33:24
 * @LastEditTime : 2026-03-26 16:42:54
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#ifndef __KERNEL_DEVICETABLE_H
#define __KERNEL_DEVICETABLE_H

struct of_device_id {
    char name[32];
    char type[32];
    char compatible[128];
    const void *data;
};

#endif