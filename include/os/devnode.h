#ifndef __OS_DEVNODE_H
#define __OS_DEVNODE_H

#include <os/types.h>
#include <os/list.h>
#include <fs/types.h>

#define DEV_CHAR 1
#define DEV_BLOCK 2

// 文件系统访问接口
struct devnode {
    const char *name;      // "console", "tty0", "sda", "sda1"
    int type;              // DEV_CHAR / DEV_BLOCK
    dev_t devt;            // major/minor
    const struct file_operations *fops;
    struct list_head list;
    void *private;
};

int devnode_register(const char *name, int type, dev_t devt, struct file_operations *fops, void *private);
struct devnode *devnode_lookup_by_name(const char *name);
struct devnode *devnode_lookup_by_devnr(dev_t devnr);
int devtmpfs_mount(const char *path);

#endif
