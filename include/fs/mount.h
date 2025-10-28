/**
 * @FilePath: /ZZZ/kernel/fs/vfs/mount.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 22:50:39
 * @LastEditTime: 2025-09-07 19:53:27
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_MOUNT_H
#define VFS_MOUNT_H

#include "vfs_types.h"

extern struct list vfs_mount_points;
extern int mount(const char *type_name, const char *bdev_name, int flags);
extern int mount_rootfs();
extern struct dentry *get_root();

#endif