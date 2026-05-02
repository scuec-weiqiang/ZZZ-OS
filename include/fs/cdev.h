#ifndef _FS_CDEV_H
#define _FS_CDEV_H

#include <os/types.h>
#include <fs/types.h>
#include <os/devnode.h>
#define CHRDEV_MAJOR 1
struct cdev {
    const char *name;                     // "console", "tty0", "null"
    dev_t devnr;                           // major/minor
    int cd_openers;
    void *private;
    struct devnode *node;
    struct list_head list;
};
int alloc_chrdev_region(dev_t *devt, unsigned int count);
int cdev_register(const char *name, dev_t devnr, struct file_operations *fops, void *private);
struct cdev* cdev_get_by_path(const char *path);
void cdev_put(struct cdev *cdev);
struct cdev *cdev_get_by_devnr(dev_t devnr);
#endif