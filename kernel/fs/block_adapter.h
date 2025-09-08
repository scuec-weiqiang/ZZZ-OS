/**
 * @FilePath: /ZZZ/kernel/fs/block_adapter.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 12:42:26
 * @LastEditTime: 2025-08-28 19:59:11
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef BLOCK_ADAPTER_H
#define BLOCK_ADAPTER_H

#include "types.h"
#include "block_device.h"

typedef struct block_adapter block_adapter_t;

extern int64_t block_adapter_register(const char* adap_name,const char* bdev_name, uint32_t fs_block_size);
extern void block_adapter_destory(block_adapter_t* adap);

extern block_adapter_t* block_adapter_open(const char *name);
extern int64_t block_adapter_read(block_adapter_t* adap, void* buf, uint64_t logic_block_start, uint64_t n);
extern int64_t block_adapter_write(block_adapter_t* adap, void* buf, uint64_t logic_block_start, uint64_t n);
extern int64_t block_adapter_get_block_size(block_adapter_t *adap);
extern int64_t block_adapter_get_sectors_per_block(block_adapter_t *adap);
#endif