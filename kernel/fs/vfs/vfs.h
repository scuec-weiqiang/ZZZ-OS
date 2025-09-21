/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 21:02:22
 * @LastEditTime: 2025-09-20 16:56:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_H
#define VFS_H

#include "vfs_types.h"
#include "fs_init.h"

extern struct dentry* lookup(const char* path);
extern struct dentry* mkdir(const char* path,u16 mode);
extern struct dentry* rmdir(const char* path);
extern struct file* open(const char *path, u32 flags);
extern ssize_t read(struct file *file, char *buf, size_t read_size);
extern ssize_t write(struct file *file, const char *buf, size_t count);

extern void vfs_test();

#endif