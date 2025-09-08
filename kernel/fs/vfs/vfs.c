/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:53
 * @LastEditTime: 2025-09-07 23:46:16
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "vfs.h"
#include "vfs_mount.h"
#include "vfs_types.h"
#include "vfs_dcache.h"
#include "vfs_path.h"

#include "check.h"
#include "string.h"

#include "vfs_icache.h"

vfs_dentry_t* current = NULL;

vfs_dentry_t* vfs_lookup(const char* path)
{
    CHECK(path != NULL, "", return NULL;);

    vfs_dentry_t *d_parent = NULL;
    vfs_dentry_t *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    if(path[0] == '/') // 根目录
    {
        d_parent = vfs_get_root();
    }
    else
    {
        d_parent = current;
    }

    char *token = path_split(path_copy, "/");
    while (token)
    {
        d_child = vfs_dget(d_parent, token);
        if(d_child->d_inode == NULL) // 目录项不存在
        {
            free(path_copy);
            return NULL;
        }
        token = path_split(NULL, "/");
        d_parent = d_child;
    }
    free(path_copy);
    return d_parent;
}

vfs_dentry_t* vfs_mkdir(const char* path,uint16_t mode)
{
    CHECK(path != NULL, "", return NULL;);

    vfs_dentry_t *d_parent = NULL;
    vfs_dentry_t *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    char* basename = malloc(strlen(path_copy) + 1);
    char* dirname = malloc(VFS_NAME_MAX + 1);

    base_dir_split(path_copy, dirname, basename);

    d_parent = vfs_lookup(dirname);
    if (d_parent == NULL) {

        d_child = NULL;
        goto exit;
    }

    d_child = vfs_dnew(d_parent, basename,NULL);

    d_parent->d_inode->i_ops->mkdir(d_parent->d_inode, d_child, mode);

exit:
    free(basename);
    free(dirname);
    free(path_copy);
    return d_child; 

}

void vfs_test()
{
    vfs_inode_t *inode1 = vfs_inew(vfs_get_root()->d_inode->i_sb);
    vfs_inode_t *inode2 = vfs_inew(vfs_get_root()->d_inode->i_sb);
    
    vfs_dentry_t* d = vfs_lookup("/a/b");

    // if (d == NULL) {
    //     printf("Directory not found.\n");
    // }
    // else
    // {
    //     printf("Directory found: %s\n", d->name.name);
    //     printf("Directory parent: %s\n", d->d_parent->name.name);
    //     printf("Directory parent parent: %s\n", d->d_parent->d_parent->name.name);
    // }

    // d = vfs_mkdir("/c/d",S_IFDIR | S_IDEFAULT);
    // if (d == NULL) {
    //     printf("Directory not create.\n");
    // }
    // else
    // {
    //     printf("Directory created: %s\n", d->name.name);
    //     printf("Directory parent: %s\n", d->d_parent->name.name);
    //     printf("Directory parent parent: %s\n", d->d_parent->d_parent->name.name);
    // }
}

vfs_file_t* vfs_open(const char *path)
{

    return 0;
}

// 读写文件
int64_t vfs_read(int64_t fd, void *buf, size_t size)
{

    return 0;
}

int64_t vfs_write(int64_t fd, const void *buf, size_t size)
{
    
    return 0;
}