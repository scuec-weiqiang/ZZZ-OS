/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 21:02:22
 * @LastEditTime: 2025-09-01 22:51:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/**
 * @FilePath: /ZZZ/kernel/fs/vfs.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 21:02:22
 * @LastEditTime: 2025-08-22 18:10:13
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_H
#define VFS_H

#include "vfs_types.h"

int64_t vfs_register_fs(vfs_fs_type_t *type);

vfs_superblock_t* vfs_mount(const char *type_name, const char *bdev_name, int64_t flags);

// vfs_inode_t* vfs_get_root(vfs_superblock_t *sb);

// // 创建文件
// int64_t vfs_create(const char *path, int64_t mode);

// // 打开文件
// int64_t vfs_open(const char *path);

// // 读写文件
// int64_t vfs_read(int64_t fd, void *buf, size_t size);
// int64_t vfs_write(int64_t fd, const void *buf, size_t size);

#endif