/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_mount.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 23:14:10
 * @LastEditTime: 2025-09-07 19:52:57
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "vfs_dcache.h"
#include "check.h"

/* 挂载点链表 */
list_t vfs_mount_points = LIST_HEAD_INIT(vfs_mount_points);

// static vfs_superblock_t *g_root_sb = NULL;
static vfs_dentry_t *g_root_dentry;

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

int64_t vfs_mount_root()
{
    CHECK(list_empty(&vfs_mount_points), "Root filesystem has already exists", return -1;);
    vfs_mount("ext2", "virt_disk", 0); 
    g_root_dentry = vfs_create_dentry("/");
    g_root_dentry->d_inode = container_of(vfs_mount_points.prev,vfs_mount_t,mnt_list)->mnt_root;
    g_root_dentry->d_parent = g_root_dentry; // 根目录的父目录是它自己
    return 0;

}

vfs_dentry_t *vfs_get_root() 
{ 
    return g_root_dentry; 
}