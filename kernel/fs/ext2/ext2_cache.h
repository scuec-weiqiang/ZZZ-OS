/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_cache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 01:22:40
 * @LastEditTime: 2025-09-01 22:05:31
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_CACHE_H
#define EXT2_CACHE_H

#include "types.h"
#include "vfs_types.h"
#include "ext2_fs.h"

extern int64_t ext2_sync_inode_bitmap_cache(vfs_superblock_t *vfs_sb);
extern int64_t ext2_load_inode_bitmap_cache(vfs_superblock_t *vfs_sb,uint64_t group);
extern int64_t ext2_sync_inode_table_cache(vfs_superblock_t *vfs_sb);
extern int64_t ext2_load_inode_table_cache(vfs_superblock_t *vfs_sb,uint64_t group);
extern int64_t ext2_select_inode_group(vfs_superblock_t *vfs_sb);

extern int64_t ext2_sync_block_bitmap_cache(vfs_superblock_t *vfs_sb);
extern int64_t ext2_load_block_bitmap_cache(vfs_superblock_t *vfs_sb,uint64_t group);
extern int64_t ext2_select_block_group(vfs_superblock_t *vfs_sb);

extern int64_t ext2_sync_cache(vfs_superblock_t *sb);
#endif