/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-23 19:20:48
 * @LastEditTime: 2025-09-07 20:14:18
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "vfs_fs_type.h"
#include "vfs_icache.h"
#include "vfs_pcache.h"
#include "vfs_mount.h"

#include "ext2_super.h"

extern vfs_inode_t *ext2_lookup(vfs_inode_t *dir, const char *name);
extern int64_t ext2_mkdir(vfs_inode_t *dir, const char *name, uint32_t i_mode);


int64_t vfs_init()
{
    vfs_register_fs(&ext2_fs_type);
    vfs_icache_init();
    vfs_pcache_init();
    vfs_dcache_init();
    vfs_mount_root();
    return 0;
}