/**
 * @FilePath: /ZZZ/kernel/fs/block_device.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 17:30:39
 * @LastEditTime: 2025-08-31 23:05:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "types.h"

typedef struct block_device block_device_t;

struct block_device {
    char     name[16];       // 设备名，比如 "sda", "vda"
    dev_t   dev;               // 设备号
    uint32_t sector_size;
    uint64_t capacity; // in bytes
    int64_t (*read)(void *buf, uint64_t block_no);
    int64_t (*write)(void *buf, uint64_t block_no);
    void *private_data;  // 指向底层驱动的设备数据（如 virtio 磁盘结构）
};


extern int64_t block_device_register(block_device_t *bdev);
extern block_device_t* block_device_open(const char* name);

#endif