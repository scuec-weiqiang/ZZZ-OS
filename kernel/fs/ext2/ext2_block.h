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

extern int ext2_bno_group(struct superblock *vfs_sb,u64 bno);
extern int ext2_alloc_bno(struct superblock *vfs_sb);
extern int ext2_release_bno(struct superblock *vfs_sb,u64 bno);
extern int ext2_block_mapping(struct inode *inode, u64 index);
#endif