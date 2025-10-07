/**
 * @FilePath: /ZZZ/kernel/fs/vfs/mount.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 23:14:10
 * @LastEditTime: 2025-09-07 19:52:57
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "vfs_types.h"
#include "fs_type.h"
#include "dcache.h"
#include "check.h"
#include "malloc.h"

/* 挂载点链表 */
struct list vfs_mount_points = LIST_HEAD_INIT(vfs_mount_points);

// static struct superblock *g_root_sb = NULL;
static struct dentry *g_root_dentry;

int mount(const char *type_name, const char *bdev_name, int flags)
{
    CHECK((type_name != NULL), "Invalid type name", return -1;);
    CHECK((bdev_name != NULL), "Invalid block device name", return -1;);

    struct fs_type *fs_type = fs_get(type_name);
    CHECK((fs_type != NULL), "Invalid fs type", return -1;);
    CHECK((fs_type->mount != NULL), "Invalid mount function", return -1;);

    struct block_device *bdev = block_device_open(bdev_name);
    CHECK((bdev != NULL), "Invalid block device", return -1;);

    struct superblock *sb = fs_type->mount(fs_type, bdev, flags);

    struct mount_point *mnt = (struct mount_point *)malloc(sizeof(struct mount_point));
    CHECK(mnt != NULL, "Failed to allocate memory for mount point", return -1;);
    mnt->mnt_sb = sb;
    mnt->mnt_root = sb->s_root;
    INIT_LIST_HEAD(&mnt->mnt_list);

    list_add(&vfs_mount_points, &mnt->mnt_list);

    return 0;
}

int mount_rootfs()
{
    CHECK(list_empty(&vfs_mount_points), "Root filesystem has already exists", return -1;);
    mount("ext2", "virt_disk", 0);
    g_root_dentry = create_dentry("/");
    g_root_dentry->d_inode = container_of(vfs_mount_points.prev, struct mount_point, mnt_list)->mnt_root;
    g_root_dentry->d_parent = g_root_dentry; // 根目录的父目录是它自己
    return 0;
}

struct dentry *get_root()
{
    return g_root_dentry;
}