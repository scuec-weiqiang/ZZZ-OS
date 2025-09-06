/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_super.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 20:52:24
 * @LastEditTime: 2025-08-31 23:59:44
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _VFS_SUPER_H
#define _VFS_SUPER_H

#include "vfs_types.h"

extern vfs_superblock_t *vfs_alloc_super(vfs_fs_type_t *fs_type);
extern void vfs_free_super(vfs_superblock_t *sb);
extern int64_t vfs_sync_super(vfs_superblock_t *sb);

#endif