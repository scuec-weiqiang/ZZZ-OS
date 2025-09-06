/**
 * @FilePath: /ZZZ/drivers/virt_disk.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:58
 * @LastEditTime: 2025-08-12 17:30:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef VIRT_DISK_H
#define VIRT_DISK_H

#include "types.h"
#include "printf.h"
#include "color.h"

#define VIRT_DISK_QUEUE 0

#define SECTOR_SIZE 512 

typedef enum {
    VIRT_DISK_READ = 0,
    VIRT_DISK_WRITE,
}virt_disk_rw_t;

extern void virt_disk_init();
extern uint64_t virt_disk_get_capacity();
extern int disk_read(void *buffer, uint64_t sector);
extern int disk_write(void *buffer, uint64_t sector);
extern int disk_read1024(void *buffer, uint64_t sector);
extern int disk_write1024(void *buffer, uint64_t sector);
extern int disknwrite(void *data, uint64_t start, uint64_t num);
extern int disknread(void *buf, uint64_t start, uint64_t num);
extern int disknwrite1024(void *data, uint64_t start, uint64_t num);
extern int disknread1024(void *buf, uint64_t start, uint64_t num);


#endif