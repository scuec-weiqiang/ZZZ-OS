/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_block.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 17:54:54
 * @LastEditTime: 2025-09-01 19:43:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_BLOCK_H
#define EXT2_BLOCK_H
#include "types.h"
#include "vfs_types.h"

extern int64_t ext2_alloc_bno(vfs_superblock_t *vfs_sb, uint64_t group);
extern int64_t ext2_release_bno(vfs_superblock_t *vfs_sb,uint64_t bno);
extern int64_t ext2_block_mapping(vfs_inode_t *vfs_inode, uint32_t index);
#endif