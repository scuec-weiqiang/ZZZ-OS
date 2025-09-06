/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-23 19:20:48
 * @LastEditTime: 2025-09-04 16:47:41
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
    vfs_mount("ext2", "virt_disk", 0); // 
    vfs_inode_t *root_inode = container_of(vfs_mount_points.next,vfs_mount_t,mnt_list)->mnt_root;
    
    // vfs_inode_t *dir = ext2_lookup(root_inode, "lost+found");
    ext2_mkdir(root_inode, "a", 0x41ed); // 创建一个目录
    
    return 0;
}