/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 21:02:22
 * @LastEditTime: 2025-09-14 13:54:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_H
#define VFS_H

#include "vfs_types.h"
#include "vfs_init.h"


extern vfs_dentry_t* vfs_lookup(const char* path);
extern vfs_dentry_t* vfs_mkdir(const char* path,uint16_t mode);
extern vfs_dentry_t* vfs_rmdir(const char* path);
extern vfs_file_t* vfs_open(const char *path, uint32_t flags);
extern ssize_t vfs_read(vfs_file_t *file, char *buf, size_t read_size);
extern ssize_t vfs_write(vfs_file_t *file, const char *buf, size_t count);

extern void vfs_test();

// int64_t vfs_register_fs(vfs_fs_type_t *type);

// vfs_superblock_t* vfs_mount(const char *type_name, const char *bdev_name, int64_t flags);

// vfs_inode_t* vfs_get_root(vfs_superblock_t *sb);

// // 创建文件
// int64_t vfs_create(const char *path, int64_t mode);

// // 打开文件
// int64_t vfs_open(const char *path);

// // 读写文件
// int64_t vfs_read(int64_t fd, void *buf, size_t size);
// int64_t vfs_write(int64_t fd, const void *buf, size_t size);

#endif