/**
 * @FilePath: /vboot/fs/block_adapter.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 12:42:19
 * @LastEditTime: 2025-09-17 21:18:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <fs/block_adapter.h>
#include <os/string.h>
#include <os/types.h>
#include <os/kmalloc.h>

#define ADAP_INVALID_ARG    -1
#define ADAP_BDEV_NULL      -2
#define ADAP_NOT_ALIGN      -3
#define ADAP_ALLOC_ERR      -4
#define ADAP_FULL           -5
#define ADAP_NOT_FOUND      -6

struct block_adapter
{
    char name[16];
    struct block_device *bdev;
    int fs_block_size;
    int sectors_per_block;
};

#define MAX_BLOCK_ADAPTER_NUM 8
struct block_adapter block_adapter_registry[MAX_BLOCK_ADAPTER_NUM];

int block_adapter_register(const char* adap_name,const char* bdev_name, uint32_t fs_block_size)
{
    // 检查文件系统块大小是否为磁盘扇区大小的整数倍
    if (adap_name == NULL || bdev_name == NULL || fs_block_size%512!=0)
    {
        return ADAP_INVALID_ARG;
    }

    struct block_device *bdev = block_device_open(bdev_name);
    if(bdev == NULL)
    {
        return ADAP_BDEV_NULL;
    }

    if(bdev->sector_size == 0 || fs_block_size % bdev->sector_size != 0)
    {
        return ADAP_NOT_ALIGN;
    }

    struct block_adapter adap;
    strcpy(adap.name,adap_name);
    adap.bdev = bdev;
    adap.fs_block_size = fs_block_size;
    adap.sectors_per_block = fs_block_size / bdev->sector_size;

    for(int i=0;i<MAX_BLOCK_ADAPTER_NUM;i++)
    {
        if(block_adapter_registry[i].name[0] == 0)
        {
            memcpy(&block_adapter_registry[i],&adap,sizeof(struct block_adapter));
            return i;
        }
    }
    return ADAP_FULL;
}

void block_adapter_destory(struct block_adapter *adap)
{
    kfree(adap);
}

struct block_adapter* block_adapter_open(const char* name)
{
    for(int i=0;i<MAX_BLOCK_ADAPTER_NUM;i++)
    {
        if(strcmp(block_adapter_registry[i].name,name) == 0)
        {
            return &block_adapter_registry[i];
        }
    }
    return NULL;
}

int block_adapter_read(struct block_adapter *adap, void *buf, int logic_block_start, int n)
{
    if (adap == NULL || buf == NULL)
    {
        return -1;
    }

    // 计算出从磁盘上的哪个扇区开始读，以及读多少个扇区
    int phy_sector_start = logic_block_start * adap->sectors_per_block;
    int phy_sector_size = adap->fs_block_size / adap->sectors_per_block;
    int phy_sector_n = n * adap->sectors_per_block;

    char *pos = (char *)buf;
    for (int i = phy_sector_start; i < phy_sector_start + phy_sector_n; i++)
    {
        int retval = adap->bdev->read(pos, i);
        if (retval < 0)
        {
            return (-1) * (i / adap->sectors_per_block); // 出现错误则返回具体是在文件系统哪一个逻辑block出现的错误
        }
        pos += phy_sector_size;
    }

    return 0;
}

int block_adapter_write(struct block_adapter *adap, void *buf, int logic_block_start, int n)
{
    if (adap == NULL || buf == NULL)
    {
        return -1;
    }

    // 计算出从磁盘上的哪个扇区开始写，以及写多少个扇区
    int phy_sector_start = logic_block_start * adap->sectors_per_block;
    int phy_sector_size = adap->fs_block_size / adap->sectors_per_block;
    int phy_sector_n = n * adap->sectors_per_block;

    char *pos = (char *)buf;
    for (int i = phy_sector_start; i < phy_sector_start + phy_sector_n; i++)
    {
        int retval = adap->bdev->write(pos, i);
        if (retval < 0)
        {
            return (-1) * (i / adap->sectors_per_block); // 出现错误则返回具体是在文件系统哪一个逻辑block出现的错误
        }
        pos += phy_sector_size;
    }

    return 0;
}

int block_adapter_get_block_size(struct block_adapter *adap)
{
    if(adap==NULL) return -1;
    return (int)adap->fs_block_size;
}

int block_adapter_get_sectors_per_block(struct block_adapter *adap)
{
    if(adap==NULL) return -1;
    return (int)adap->sectors_per_block;
}