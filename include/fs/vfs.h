/**
 * @FilePath: /ZZZ-OS/fs/fs/vfs.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 21:02:22
 * @LastEditTime: 2025-10-09 23:57:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_H
#define VFS_H

#include <fs/vfs_types.h>

extern struct dentry* lookup(const char* path);
extern struct dentry* mkdir(const char* path,uint16_t mode);
extern struct dentry* rmdir(const char* path);
extern struct dentry* mknod(const char* path,uint16_t mode, dev_t dev);
extern struct dentry* creat(const char* path,uint16_t mode);
extern void close(struct file *file);
extern ssize_t read(struct file *file, char *buf, size_t read_size);
extern ssize_t write(struct file *file, const char *buf, size_t count);
extern struct file* open(const char *path, uint32_t flags);
extern void vfs_test();

#endif