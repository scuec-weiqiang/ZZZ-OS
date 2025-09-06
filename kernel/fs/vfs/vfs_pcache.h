/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_pcache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 17:20:59
 * @LastEditTime: 2025-09-03 22:26:47
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_PCACHE_H
#define VFS_PCACHE_H

#include "types.h"
#include "vfs_types.h"

extern int64_t vfs_pcache_init(void);
extern void vfs_pcache_destory(void);
extern vfs_page_t *vfs_create_page(vfs_inode_t *inode, pgoff_t index) ;

extern vfs_page_t *vfs_pget(vfs_inode_t *inode, uint32_t index);
extern int64_t vfs_pput(vfs_page_t *page);
extern int64_t vfs_pcache_sync();

#endif