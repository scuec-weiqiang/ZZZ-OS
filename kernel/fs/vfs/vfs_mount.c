/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_mount.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 23:14:10
 * @LastEditTime: 2025-09-01 22:59:35
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "check.h"

/* 挂载点链表 */
list_t vfs_mount_points = LIST_HEAD_INIT(vfs_mount_points);

// static vfs_superblock_t *g_root_sb = NULL;
// static vfs_inode_t *g_root_inode= NULL;

int64_t vfs_mount(const char *type_name, const char *bdev_name, int64_t flags)
{
    CHECK( (type_name != NULL), "Invalid type name", return NULL;);
    CHECK( (bdev_name != NULL), "Invalid block device name", return NULL;);

    vfs_fs_type_t *fs_type = vfs_get_fs(type_name);
    CHECK( (fs_type != NULL), "Invalid fs type", return NULL;);
    CHECK( (fs_type->mount != NULL), "Invalid mount function", return NULL;);

    block_device_t *bdev = block_device_open(bdev_name);
    CHECK( (bdev != NULL), "Invalid block device", return NULL;);

    vfs_superblock_t *sb = fs_type->mount(fs_type, bdev, flags);;

    vfs_mount_t *mnt = (vfs_mount_t *)malloc(sizeof(vfs_mount_t));
    CHECK(mnt != NULL, "Failed to allocate memory for mount point", return -1;);
    mnt->mnt_sb = sb;
    mnt->mnt_root = sb->s_root;
    INIT_LIST_HEAD(&mnt->mnt_list);

    list_add(&vfs_mount_points,&mnt->mnt_list);

    return 0;
}

// int64_t vfs_set_root(vfs_superblock_t *sb)
// {
//     CHECK(sb != NULL, "Invalid sb", return -1;);
//     CHECK(sb->s_root != NULL, "Invalid root inode", return -1;);

//     g_root_sb = sb;
//     g_root_inode = sb ? sb->s_root : NULL;

//     return 0;
// }

// vfs_inode_t *vfs_get_root(void) 
// { 
//     return g_root_inode; 
// }