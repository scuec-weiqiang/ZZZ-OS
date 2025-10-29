/**
 * @FilePath: /ZZZ-OS/fs/fs/icache.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 00:53:13
 * @LastEditTime: 2025-10-06 18:16:42
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef VFS_CACHE_H
#define VFS_CACHE_H
#include "os/types.h"
#include "os/lru.h"
#include "fs/vfs_types.h"

extern struct lru_cache *global_inode_cache;

extern int icache_init();
extern void icache_destroy();

extern struct inode *create_inode(struct superblock *sb);

extern struct inode *inew(struct superblock *sb);
extern struct inode *iget(struct superblock *sb, ino_t ino);
extern int iput(struct inode *inode);

extern int icache_sync(struct inode *inode);
extern int icache_sync_all();
#endif