/**
 * @FilePath: /vboot/fs/fs/block_device.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 17:30:39
 * @LastEditTime: 2025-09-17 20:52:17
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "os/types.h"

struct block_device {
    char     name[16];       // 设备名，比如 "sda", "vda"
    int   dev;               // 设备号
    int sector_size;
    int capacity; // in bytes
    int (*read)(void *buf, int block_no);
    int (*write)(void *buf, int block_no);
    void *private_data;  // 指向底层驱动的设备数据（如 virtio 磁盘结构）
};


extern int block_device_register(struct block_device *bdev);
extern struct block_device* block_device_open(const char* name);

#endif