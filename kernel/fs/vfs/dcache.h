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

extern int dcache_init();
extern void dcache_destory();

extern struct dentry* vfs_create_dentry(const char *name);

extern struct dentry* dnew(struct dentry *parent, const char *name, struct inode *inode);
extern struct dentry *dget(struct dentry *parent, const char *name);
extern int dput(struct dentry *dentry);

#endif