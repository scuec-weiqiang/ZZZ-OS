/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_super.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 20:20:03
 * @LastEditTime: 2025-08-28 20:20:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_SUPER_H
#define EXT2_SUPER_H

#include "types.h"
#include "vfs_types.h"

extern vfs_fs_type_t ext2_fs_type;
extern int64_t ext2_sync_super(vfs_superblock_t *sb);

#endif