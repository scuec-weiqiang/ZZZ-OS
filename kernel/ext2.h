/**
 * @FilePath: /ZZZ/kernel/ext2.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-31 15:28:59
 * @LastEditTime: 2025-06-29 22:12:20
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __EXT2_H__
#define __EXT2_H__

#include "types.h"

#define EXT2_SUPER_MAGIC 0xEF53

typedef struct ext2_fs ext2_fs_t;

extern int file_system_test();
extern ext2_fs_t* ext2_fs_create();
extern int64_t ext2_fs_destroy(ext2_fs_t *fs);
extern int64_t ext2_fs_format(ext2_fs_t *fs);
extern int64_t ext2_fs_load(ext2_fs_t *fs);

extern int64_t ext2_create_dir_by_path(ext2_fs_t *fs, const char *path);// 创建目录
extern int64_t ext2_create_file_by_path(ext2_fs_t *fs, const char *path); // 创建文件
extern int64_t ext2_overwrite_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size); // 覆盖写
extern int64_t ext2_append_file_by_path(ext2_fs_t *fs, const char *path, const void *data, uint64_t size); // 追加写
extern int64_t ext2_read_file_by_path(ext2_fs_t *fs, const char *path, void *buf); // 读取
extern int64_t ext2_unlink_by_path(ext2_fs_t *fs, const char *path);
extern int64_t ext2_get_file_size_by_path(ext2_fs_t *fs, const char *path); // 查询文件大小
extern int64_t ext2_list_dir_by_path(ext2_fs_t *fs,const char *path);
#endif