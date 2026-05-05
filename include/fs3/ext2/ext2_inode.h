/**
 * @FilePath: /ZZZ/kernel/fs/ext2/fs/ext2/ext2_inode.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-02 18:27:09
 * @LastEditTime: 2025-09-04 16:32:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_INODE_H
#define EXT2_INODE_H

#include <os/types.h>
#include <fs/types.h>

extern int ext2_ino_group(struct super_block *vfs_sb, u32 ino);
extern u32 ext2_alloc_ino(struct super_block *vfs_sb);
extern int ext2_release_ino(struct super_block *vfs_sb, u32 ino);

#endif