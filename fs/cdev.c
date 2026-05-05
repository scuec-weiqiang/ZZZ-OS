#include <fs/cdev.h>
#include <os/err.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/printk.h>
static LIST_HEAD(g_cdevs);

int alloc_chrdev_region(dev_t *devt, unsigned int count) {
    static dev_t next_devt = 1; // 从 1 开始分配，0 通常保留给特殊用途

    if (!devt || count == 0)
        return -EINVAL;

    *devt = MKDEV(CHRDEV_MAJOR, next_devt);
    next_devt += count;
    return 0;
}

int cdev_register(const char *name, dev_t devnr, const struct file_operations *fops, void *private) {
    struct cdev *cdev;

    struct cdev *iter = NULL;

    list_for_each_entry(iter, &g_cdevs, struct cdev, list) {
        if (strcmp(iter->name, name) == 0) {
            
            return -EEXIST;
        }
    }

    cdev = kzalloc(sizeof(*cdev));
    if (!cdev)
        return -ENOMEM;

    cdev->name = strdup(name);
    cdev->devnr = devnr;
    cdev->private = private;

    int ret;

    if (!cdev || !cdev->name)
        return -EINVAL;

    ret = devnode_register(cdev->name, DEV_CHAR, devnr, fops, cdev);
    if (ret < 0) {
        
        kfree(cdev);
         return ret;
    }
    cdev->node = devnode_lookup_by_name(cdev->name);
    if (!cdev->node) {
        
        kfree(cdev);
        return -EINVAL;
    }
    
    list_add_tail( &g_cdevs,&cdev->list);
    return 0;
}

struct cdev* cdev_get_by_path(const char *path) {
    struct devnode *node;
    struct cdev *cdev;

    if (!path)
        return ERR_PTR(-EINVAL);

    if (strncmp(path, "/dev/", 5) == 0)
        path += 5;

    node = devnode_lookup_by_name(path);
    if (!node)
        return ERR_PTR(-ENODEV);

    if (node->type != DEV_CHAR)
        return ERR_PTR(-ENODEV);

    cdev = node->private;
    if (!cdev)
        return ERR_PTR(-ENODEV);

    return cdev;
}



struct cdev *cdev_get_by_devnr(dev_t devnr)
{
    struct list_head *pos;

    list_for_each(pos, &g_cdevs) {
        struct cdev *cdev = container_of(pos, struct cdev, list);
        if (cdev->devnr == devnr) {
            return cdev;
        }
    }

    return NULL;
}

void cdev_put(struct cdev *cdev) {
    if (cdev == NULL) {
        return;
    }

    if (cdev->cd_openers > 0) {
        cdev->cd_openers--;
    }
}
