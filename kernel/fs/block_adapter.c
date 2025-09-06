/**
 * @FilePath: /ZZZ/kernel/fs/block_adapter.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 12:42:19
 * @LastEditTime: 2025-08-28 19:59:02
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "block_adapter.h"
#include "string.h"
#include "types.h"
#include "page_alloc.h"

#define ADAP_INVALID_ARG    -1
#define ADAP_BDEV_NULL      -2
#define ADAP_NOT_ALIGN      -3
#define ADAP_ALLOC_ERR      -4
#define ADAP_FULL           -5
#define ADAP_NOT_FOUND      -6

typedef struct block_adapter block_adapter_t;

struct block_adapter
{
    char* name[16];
    struct block_device *bdev;
    uint32_t fs_block_size;
    uint32_t sectors_per_block;

    // 简单缓存
    uint32_t cached_block;
    uint8_t *cached_data;
};

#define MAX_BLOCK_ADAPTER_NUM 8
block_adapter_t block_adapter_registry[MAX_BLOCK_ADAPTER_NUM];

int64_t block_adapter_register(const char* adap_name,const char* bdev_name, uint32_t fs_block_size)
{
    // 检查文件系统块大小是否为磁盘扇区大小的整数倍
    if (adap_name == NULL || bdev_name == NULL || fs_block_size%512!=0)
    {
        return ADAP_INVALID_ARG;
    }

    block_device_t *bdev = block_device_open(bdev_name);
    if(bdev == NULL)
    {
        return ADAP_BDEV_NULL;
    }

    if(bdev->sector_size == 0 || fs_block_size % bdev->sector_size != 0)
    {
        return ADAP_NOT_ALIGN;
    }

    block_adapter_t adap;
    strcpy(adap.name,adap_name);
    adap.bdev = bdev;
    adap.fs_block_size = fs_block_size;
    adap.sectors_per_block = fs_block_size / bdev->sector_size;

    // 暂时不启用缓存机制
    adap.cached_block = 0;
    adap.cached_data = NULL;

    for(uint64_t i=0;i<MAX_BLOCK_ADAPTER_NUM;i++)
    {
        if(block_adapter_registry[i].name[0] == 0)
        {
            memcpy(&block_adapter_registry[i],&adap,sizeof(block_adapter_t));
            return i;
        }
    }
    return ADAP_FULL;
}

void block_adapter_destory(block_adapter_t *adap)
{
    free(adap);
}

block_adapter_t* block_adapter_open(const char* name)
{
    for(uint64_t i=0;i<MAX_BLOCK_ADAPTER_NUM;i++)
    {
        if(strcmp(block_adapter_registry[i].name,name) == 0)
        {
            return &block_adapter_registry[i];
        }
    }
    return NULL;
}

int64_t block_adapter_read(block_adapter_t *adap, void *buf, uint64_t logic_block_start, uint64_t n)
{
    if (adap == NULL || buf == NULL)
    {
        return -1;
    }

    // 计算出从磁盘上的哪个扇区开始读，以及读多少个扇区
    uint64_t phy_sector_start = logic_block_start * adap->sectors_per_block;
    uint64_t phy_sector_size = adap->fs_block_size / adap->sectors_per_block;
    uint64_t phy_sector_n = n * adap->sectors_per_block;

    uint8_t *pos = (uint8_t *)buf;
    for (uint64_t i = phy_sector_start; i < phy_sector_start + phy_sector_n; i++)
    {
        int64_t retval = adap->bdev->read(pos, i);
        if (retval < 0)
        {
            return (-1) * (i / adap->sectors_per_block); // 出现错误则返回具体是在文件系统哪一个逻辑block出现的错误
        }
        pos += phy_sector_size;
    }

    return 0;
}

int64_t block_adapter_write(block_adapter_t *adap, void *buf, uint64_t logic_block_start, uint64_t n)
{
    if (adap == NULL || buf == NULL)
    {
        return -1;
    }

    // 计算出从磁盘上的哪个扇区开始写，以及写多少个扇区
    uint64_t phy_sector_start = logic_block_start * adap->sectors_per_block;
    uint64_t phy_sector_size = adap->fs_block_size / adap->sectors_per_block;
    uint64_t phy_sector_n = n * adap->sectors_per_block;

    uint8_t *pos = (uint8_t *)buf;
    for (uint64_t i = phy_sector_start; i < phy_sector_start + phy_sector_n; i++)
    {
        int64_t retval = adap->bdev->write(pos, i);
        if (retval < 0)
        {
            return (-1) * (i / adap->sectors_per_block); // 出现错误则返回具体是在文件系统哪一个逻辑block出现的错误
        }
        pos += phy_sector_size;
    }

    return 0;
}

int64_t block_adapter_get_block_size(block_adapter_t *adap)
{
    if(adap==NULL) return -1;
    return (uint64_t)adap->fs_block_size;
}

int64_t block_adapter_get_sectors_per_block(block_adapter_t *adap)
{
    if(adap==NULL) return -1;
    return (uint64_t)adap->sectors_per_block;
}