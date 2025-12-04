/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/fs/vfs.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:53
 * @LastEditTime: 2025-10-22 22:56:52
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include <fs/vfs.h>
#include <fs/mount.h>
#include <fs/vfs_types.h>
#include <fs/dcache.h>
#include <fs/path.h>

#include <os/check.h>
#include <os/string.h>

#include <fs/icache.h>
#include <os/kmalloc.h>
#include <fs/chrdev.h>

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
            kfree(path_copy);
            return NULL;
        }
        token = path_split(NULL, "/");
        d_parent = d_child;
    }
    kfree(path_copy);
    return d_parent;
}

struct dentry* mkdir(const char* path,uint16_t mode)
{
    CHECK(path != NULL, "", return NULL;);

    struct dentry *d_parent = NULL;
    struct dentry *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    char* basename = (char*)kmalloc(strlen(path_copy) + 1);
    char* dirname = (char*)kmalloc(VFS_NAME_MAX + 1);

    base_dir_split(path_copy, dirname, basename);

    d_parent = lookup(dirname);
    if (d_parent == NULL) {

        d_child = NULL;
        goto exit;
    }

    d_child = dget(d_parent, basename);

    d_parent->d_inode->i_ops->mkdir(d_parent->d_inode, d_child, mode);

exit:
    kfree(basename);
    kfree(dirname);
    kfree(path_copy);
    return d_child; 

}

struct dentry* rmdir(const char* path)
{
//     CHECK(path != NULL, "", return NULL;);

//     struct dentry *d_parent = NULL;
//     struct dentry *d_child = NULL;

//     char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

//     char* basename = kmalloc(strlen(path_copy) + 1);
//     char* dirname = kmalloc(VFS_NAME_MAX + 1);

//     base_dir_split(path_copy, dirname, basename);

//     d_parent = lookup(dirname);
//     if (d_parent == NULL) {

//         d_child = NULL;
//         goto exit;
//     }

//     d_child = dget(d_parent, basename);

//     d_parent->d_inode->i_ops->rmdir(d_parent->d_inode, d_child);

// exit:
//     kfree(basename);
//     kfree(dirname);
//     kfree(path_copy);
//     return d_child; 
return NULL;
}

struct dentry* mknod(const char* path,uint16_t mode,dev_t devnr)
{
    CHECK(path != NULL, "", return NULL;);

    struct dentry *d_parent = NULL;
    struct dentry *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    char* basename = (char*)kmalloc(strlen(path_copy) + 1);
    char* dirname = (char*)kmalloc(VFS_NAME_MAX + 1);

    base_dir_split(path_copy, dirname, basename);

    d_parent = lookup(dirname);
    if (d_parent == NULL) {

        d_child = NULL;
        goto exit;
    }

    d_child = dget(d_parent, basename);

    d_parent->d_inode->i_ops->mknod(d_parent->d_inode, d_child, mode, devnr);

exit:
    kfree(basename);
    kfree(dirname);
    kfree(path_copy);
    return d_child; 

}

struct dentry* creat(const char* path,uint16_t mode)
{
    CHECK(path != NULL, "", return NULL;);

    struct dentry *d_parent = NULL;
    struct dentry *d_child = NULL;

    char* path_copy = strdup(path); // 复制路径字符串，因为path_split会修改原字符串

    char* basename = (char*)kmalloc(strlen(path_copy) + 1);
    char* dirname = (char*)kmalloc(VFS_NAME_MAX + 1);

    base_dir_split(path_copy, dirname, basename);

    d_parent = lookup(dirname);
    if (d_parent == NULL) {

        d_child = NULL;
        goto exit;
    }

    d_child = dget(d_parent, basename);
    d_parent->d_inode->i_ops->creat(d_parent->d_inode, d_child, mode);

exit:
    kfree(basename);
    kfree(dirname);
    kfree(path_copy);
    return d_child; 
}

struct file* open(const char *path, uint32_t flags)
{
    struct dentry *dentry = lookup(path);
    struct file *file = (struct file*)kmalloc(sizeof(struct file));
    file->f_dentry = dentry;
    file->f_inode = dentry->d_inode;
    if(S_ISCHR(file->f_inode->i_mode))
    {
       file->f_inode->f_ops = get_chr_fops(file->f_inode->i_rdev);
    }
    if(file->f_inode->f_ops == NULL)
    {
        kfree(file);
        return NULL;
    }
    if (file->f_inode->f_ops->open != NULL)
        file->f_inode->f_ops->open(dentry->d_inode,file);
    file->f_flags = flags;
    file->f_mode = dentry->d_inode->i_mode;
    file->f_pos = 0;
    file->f_refcount ++;
    return file;
}

void close(struct file *file) 
{
    CHECK(file != NULL, "", return;);
    CHECK(file->f_inode != NULL, "", return;);
    CHECK(file->f_inode->f_ops != NULL, "", return;);

    file->f_refcount --;
    if(file->f_refcount > 0)
    {
        return;
    }

    dput(file->f_dentry);
    iput(file->f_inode);
    // pput(file->f_inode->i_mapping->cached_pages);
    kfree(file);
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
    
    
    ssize_t ret =  file->f_inode->f_ops->write(file->f_inode, buf, count, &file->f_pos);
    if(ret >= 0)
    {
        return ret;
    }
    else
    {
        return -1;
    }

}


