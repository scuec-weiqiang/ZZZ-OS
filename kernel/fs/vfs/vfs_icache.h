/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_icache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 00:53:13
 * @LastEditTime: 2025-09-07 21:25:33
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_CACHE_H
#define VFS_CACHE_H
#include "types.h"
#include "lru.h"
#include "vfs_types.h"

extern lru_cache_t *global_inode_cache;

extern int64_t      vfs_icache_init();
extern void         vfs_icache_destory();

extern vfs_inode_t* vfs_create_inode(vfs_superblock_t *sb);

extern vfs_inode_t* vfs_inew(vfs_superblock_t *sb);
extern vfs_inode_t* vfs_iget(vfs_superblock_t *sb, vfs_ino_t ino);
extern int64_t      vfs_iput(vfs_inode_t *inode);

extern int64_t      vfs_icache_sync();
#endif