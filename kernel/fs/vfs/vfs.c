/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:53
 * @LastEditTime: 2025-09-14 14:20:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "vfs.h"
#include "mount.h"
#include "vfs_types.h"
#include "dcache.h"
#include "path.h"

#include "check.h"
#include "string.h"

#include "icache.h"
#include "malloc.h"

struct dentry* current = NULL;

struct dentry* lookup(const char* path)
{
    CHECK(path != NULL, "", return NULL;);

    struct dentry *d_parent = NULL;
    struct dentry *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    if(path[0] == '/') // 根目录
    {
        d_parent = get_root();
    }
    else
    {
        d_parent = current;
    }

    char *token = path_split(path_copy, "/");
    while (token)
    {
        d_child = dget(d_parent, token);
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

struct dentry* mkdir(const char* path,u16 mode)
{
    CHECK(path != NULL, "", return NULL;);

    struct dentry *d_parent = NULL;
    struct dentry *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    char* basename = (char*)malloc(strlen(path_copy) + 1);
    char* dirname = (char*)malloc(VFS_NAME_MAX + 1);

    base_dir_split(path_copy, dirname, basename);

    d_parent = lookup(dirname);
    if (d_parent == NULL) {

        d_child = NULL;
        goto exit;
    }

    d_child = dnew(d_parent, basename,NULL);

    d_parent->d_inode->i_ops->mkdir(d_parent->d_inode, d_child, mode);

exit:
    free(basename);
    free(dirname);
    free(path_copy);
    return d_child; 

}

struct dentry* rmdir(const char* path)
{
//     CHECK(path != NULL, "", return NULL;);

//     struct dentry *d_parent = NULL;
//     struct dentry *d_child = NULL;

//     char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

//     char* basename = malloc(strlen(path_copy) + 1);
//     char* dirname = malloc(VFS_NAME_MAX + 1);

//     base_dir_split(path_copy, dirname, basename);

//     d_parent = lookup(dirname);
//     if (d_parent == NULL) {

//         d_child = NULL;
//         goto exit;
//     }

//     d_child = dget(d_parent, basename);

//     d_parent->d_inode->i_ops->rmdir(d_parent->d_inode, d_child);

// exit:
//     free(basename);
//     free(dirname);
//     free(path_copy);
//     return d_child; 
return NULL;
}

struct file* open(const char *path, u32 flags)
{
    struct dentry *dentry = lookup(path);
    struct file *file = (struct file*)malloc(sizeof(struct file));
    file->f_dentry = dentry;
    file->f_inode = dentry->d_inode;
    file->f_flags = flags;
    file->f_mode = dentry->d_inode->i_mode;
    file->f_pos = 0;
    file->f_refcount ++;
    return file;
}


ssize_t read(struct file *file, char *buf, size_t read_size) 
{
    CHECK(file != NULL && buf != NULL, "", return -1;);
    CHECK(file->f_inode != NULL, "", return -1;);
    CHECK(file->f_inode->f_ops != NULL && file->f_inode->f_ops->read != NULL, "", return -1;);

    loff_t pos = file->f_pos;
    ssize_t ret = file->f_inode->f_ops->read(file->f_inode, buf, read_size, &pos);
    if(ret >= 0)
    {
        file->f_pos = pos;
        return ret;
    }
    else
    {
        return -1;
    }
}

ssize_t write(struct file *file, const char *buf, size_t count) 
{    
    CHECK(file != NULL && buf != NULL, "", return -1;);
    CHECK(file->f_inode != NULL, "", return -1;);
    CHECK(file->f_inode->f_ops != NULL && file->f_inode->f_ops->write != NULL, "", return -1;);
    
    return file->f_inode->f_ops->write(file->f_inode, buf, count, &file->f_pos);
}

void vfs_test()
{

    // struct dentry* d = look_up("/hello.txt");
    // struct dentry* d;
    // d = vfs_mkdir("/c",S_IFDIR | S_IDEFAULT);
    // d = vfs_mkdir("/d.txt",S_IFREG | S_IDEFAULT);

}

