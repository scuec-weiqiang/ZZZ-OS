/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_mount.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 22:50:39
 * @LastEditTime: 2025-09-01 22:57:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_MOUNT_H
#define VFS_MOUNT_H

#include "vfs_types.h"

extern list_t vfs_mount_points;
extern int64_t vfs_mount(const char *type_name, const char *bdev_name, int64_t flags);

#endif;