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

#include "types.h"
#include "vfs_types.h"

extern int64_t          vfs_register_fs(vfs_fs_type_t *fs_type);
extern int64_t          vfs_unregister_fs(vfs_fs_type_t *fs_type);
extern vfs_fs_type_t*   vfs_get_fs(const char* name);

#endif