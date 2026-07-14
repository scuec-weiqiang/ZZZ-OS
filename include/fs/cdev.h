#ifndef _FS_CDEV_H
#define _FS_CDEV_H

#include <os/types.h>
#include <fs/types.h>
#include <os/devnode.h>
#include <os/spinlock.h>

#define CHRDEV_MAJOR 1

struct cdev {
    const char *name;                     // "console", "tty0", "null"
    dev_t devnr;                           // major/minor
    int cd_openers;
    void *private;
    const struct file_operations *fops;
    struct devnode *node;
    int count;
    struct list_head list;
};


struct chrdev_region {
    dev_t from;
    unsigned int count;

    char name[32];

    struct list_head node;
};

struct chrdev_registry {
    spinlock_t lock;
    struct list_head regions;

    unsigned int dynamic_major_start;
    unsigned int dynamic_major_end;
};

int register_chrdev_region(dev_t from,
                           unsigned int count,
                           const char *name);

int alloc_chrdev_region(dev_t *dev,
                        unsigned int first_minor,
                        unsigned int count,
                        const char *name);

void unregister_chrdev_region(dev_t from,
                              unsigned int count);
struct cdev *cdev_alloc();

int cdev_add(struct cdev *cdev, dev_t devnr, int count);
int cdev_register(const char *name, dev_t devnr, int count,
                    const struct file_operations *fops, 
                    void *private);
struct cdev* cdev_get_by_path(const char *path);
void cdev_put(struct cdev *cdev);
struct cdev *cdev_get_by_devnr(dev_t devnr);
#endif