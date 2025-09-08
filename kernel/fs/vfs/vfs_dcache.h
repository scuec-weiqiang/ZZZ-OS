/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_dcache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-07 14:26:49
 * @LastEditTime: 2025-09-07 22:12:49
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_DCACHE_H
#define VFS_DCACHE_H

#include "types.h"
#include "vfs_types.h"

extern int64_t vfs_dcache_init();
extern void vfs_dcache_destory();

extern vfs_dentry_t* vfs_create_dentry(const char *name);

extern vfs_dentry_t* vfs_dnew(vfs_dentry_t *parent, const char *name, vfs_inode_t *inode);
extern vfs_dentry_t *vfs_dget(vfs_dentry_t *parent, const char *name);
extern int64_t vfs_dput(vfs_dentry_t *dentry);

#endif