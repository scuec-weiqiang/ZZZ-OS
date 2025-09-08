/**
 * @FilePath: /ZZZ/kernel/fs/block_device.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 17:41:28
 * @LastEditTime: 2025-08-13 21:08:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "block_device.h"
#include "string.h"

#define MAX_BLOCK_DEVICE_NUM 8

block_device_t bdev_registry[MAX_BLOCK_DEVICE_NUM];

int64_t block_device_register(block_device_t *bdev)
{
    for (int64_t i = 0; i < MAX_BLOCK_DEVICE_NUM; i++)
    {
        if (bdev_registry[i].name[0] == 0)
        {
            memcpy(&bdev_registry[i], bdev, sizeof(block_device_t));
            return i;
        }
    }
    return -1;
}

block_device_t* block_device_open(const char* name)
{
    for (int64_t i = 0; i < MAX_BLOCK_DEVICE_NUM; i++)
    {
        if (strcmp(bdev_registry[i].name,name) == 0)
        {
            return &bdev_registry[i];
        }
    }
    return NULL;
}
