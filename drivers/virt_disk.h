/**
 * @FilePath: /ZZZ/drivers/virt_disk.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:58
 * @LastEditTime: 2025-05-28 01:46:38
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef VIRT_DISK_H
#define VIRT_DISK_H

#include "types.h"

#define VIRT_DISK_QUEUE 0

#define BLOCK_SIZE 512 

typedef enum {
    VIRT_DISK_READ = 0,
    VIRT_DISK_WRITE,
}virt_disk_rw_t;

extern void virt_disk_init(void);
extern int virt_disk_rw(void *buffer, uint64_t sector, virt_disk_rw_t rwflag);

#endif