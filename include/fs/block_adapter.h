/**
 * @FilePath: /vboot/fs/fs/block_adapter.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 12:42:26
 * @LastEditTime: 2025-09-17 21:12:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BLOCK_ADAPTER_H
#define BLOCK_ADAPTER_H

#include "os/types.h"
#include "fs/block_device.h"

struct block_adapter;

extern int block_adapter_register(const char* adap_name,const char* bdev_name, uint32_t fs_block_size);
extern void block_adapter_destory(struct block_adapter* adap);

extern struct block_adapter* block_adapter_open(const char *name);
extern int block_adapter_read(struct block_adapter* adap, void* buf, int logic_block_start, int n);
extern int block_adapter_write(struct block_adapter* adap, void* buf, int logic_block_start, int n);
extern int block_adapter_get_block_size(struct block_adapter *adap);
extern int block_adapter_get_sectors_per_block(struct block_adapter *adap);
#endif