/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_inode.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-02 18:27:09
 * @LastEditTime: 2025-09-04 16:32:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_INODE_H
#define EXT2_INODE_H

#include "types.h"
#include "vfs_types.h"

extern u64 ext2_ino_group(struct superblock *vfs_sb,u64 ino);
extern struct super_ops ext2_s_ops;

#endif