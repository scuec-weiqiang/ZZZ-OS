/**
 * @FilePath: /ZZZ-OS/fs/ext2/fs/ext2/ext2_cache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 01:22:40
 * @LastEditTime: 2025-10-29 22:12:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_CACHE_H
#define EXT2_CACHE_H

#include "os/types.h"
#include "fs/vfs_types.h"
#include "fs/ext2/ext2_types.h"

extern int ext2_sync_inode_bitmap_cache(struct superblock *vfs_sb);
extern int ext2_load_inode_bitmap_cache(struct superblock *vfs_sb,uint64_t group);
extern int ext2_sync_inode_table_cache(struct superblock *vfs_sb);
extern int ext2_load_inode_table_cache(struct superblock *vfs_sb,uint64_t group);
extern int ext2_select_inode_group(struct superblock *vfs_sb);

extern int ext2_sync_block_bitmap_cache(struct superblock *vfs_sb);
extern int ext2_load_block_bitmap_cache(struct superblock *vfs_sb,uint64_t group);
extern int ext2_select_block_group(struct superblock *vfs_sb);

extern int ext2_sync_cache(struct superblock *sb);
#endif