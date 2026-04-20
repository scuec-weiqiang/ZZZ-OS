/**
 * @FilePath: /ZZZ-OS/fs/fs/pcache.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 17:20:59
 * @LastEditTime: 2025-10-06 18:42:47
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_PCACHE_H
#define VFS_PCACHE_H

#include <os/types.h>
#include <fs/vfs_types.h>

extern int pcache_init(void);
extern void pcache_destory(void);
extern struct page_cache *create_page(struct inode *inode, pgoff_t index) ;

extern struct page_cache *pget(struct inode *inode, uint32_t index);
extern int pput(struct page_cache *page);
extern int pcache_sync(struct page_cache *page);
extern int pcache_sync_all();

#endif