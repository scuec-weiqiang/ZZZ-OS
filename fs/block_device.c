/**
 * @FilePath: /vboot/fs/block_device.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 17:41:28
 * @LastEditTime: 2025-09-17 20:50:20
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <fs/block_device.h>
#include <os/string.h>

#define MAX_BLOCK_DEVICE_NUM 8

struct block_device bdev_registry[MAX_BLOCK_DEVICE_NUM];

int block_device_register(struct block_device *bdev)
{
    for (int i = 0; i < MAX_BLOCK_DEVICE_NUM; i++)
    {
        if (bdev_registry[i].name[0] == 0)
        {
            memcpy(&bdev_registry[i], bdev, sizeof(struct block_device));
            return i;
        }
    }
    return -1;
}

struct block_device* block_device_open(const char* name)
{
    for (int i = 0; i < MAX_BLOCK_DEVICE_NUM; i++)
    {
        if (strcmp(bdev_registry[i].name,name) == 0)
        {
            return &bdev_registry[i];
        }
    }
    return NULL;
}
