/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_path.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 00:26:04
 * @LastEditTime: 2025-09-07 17:18:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VFS_PATH_H
#define VFS_PATH_H

#include <os/types.h>

extern char *path_split(char *str, const char *delim);
extern int base_dir_split(const char *path,char *dirname, char *basename);

#endif // VFS_NAMEI_H