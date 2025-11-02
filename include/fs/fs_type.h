/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_fs_type.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 20:47:02
 * @LastEditTime: 2025-08-31 22:41:59
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _VFS_FS_TYPE_H
#define _VFS_FS_TYPE_H

#include <os/types.h>
#include <fs/vfs_types.h>

extern int          fs_register(struct fs_type *fs_type);
extern int          fs_unregister(struct fs_type *fs_type);
extern struct fs_type*   fs_get(const char* name);

#endif