/**
 * @FilePath: /ZZZ-OS/fs/fs/chrdev.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-09 21:14:01
 * @LastEditTime: 2025-10-09 23:57:59
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _CHRDEV_H
#define _CHRDEV_H

#include <fs/vfs_types.h>

struct chrdev
{
    dev_t major;
    const char *name;
    struct file_ops *fops;
};

extern int register_chrdev(dev_t major, const char *name, const struct file_ops *fops);
extern int unregister_chrdev(dev_t major, const char *name);
extern struct file_ops *get_chr_fops(dev_t major);

#endif /* _CHRDEV_H */