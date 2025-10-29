/**
 * @FilePath: /vboot/drivers/virt_disk.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:58
 * @LastEditTime: 2025-09-17 21:21:41
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef VIRT_DISK_H
#define VIRT_DISK_H

#define VIRT_DISK_QUEUE 0

#define SECTOR_SIZE 512 

enum virt_disk_rw{
    VIRT_DISK_READ = 0,
    VIRT_DISK_WRITE,
};

extern int virt_disk_init();
extern int virt_disk_get_capacity();



#endif