#include <fs/cdev.h>
#include <os/err.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/list.h>

static LIST_HEAD(g_cdevs);

static struct chrdev_registry chrdev_registry = {
    .lock = SPINLOCK_INIT,
    .regions = LIST_HEAD_INIT(chrdev_registry.regions),

    /*
     * 动态 major 从高地址向低地址分配，
     * 和静态保留的低 major 分开。
     */
    .dynamic_major_start = 4095,
    .dynamic_major_end = 256,
};

// 不给跨major注册minor
static int chrdev_range_valid(dev_t from, unsigned int count)
{
    unsigned int first_minor;
    unsigned int last_minor;

    if (count == 0)
        return 0;

    first_minor = MINOR(from);

    if (count - 1 > MINORMASK - first_minor)
        return 0;

    last_minor = first_minor + count - 1;

    return last_minor <= MINORMASK;
}

static bool chrdev_major_in_use(unsigned int major)
{
    struct chrdev_region *iter;

    list_for_each_entry(iter, &chrdev_registry.regions, node) {
        if (MAJOR(iter->from) == major)
            return true;
    }

    return false;
}

static bool chrdev_region_overlap(dev_t from1,
                                  unsigned int count1,
                                  dev_t from2,
                                  unsigned int count2)
{
    unsigned int major1 = MAJOR(from1);
    unsigned int major2 = MAJOR(from2);

    unsigned int start1 = MINOR(from1);
    unsigned int start2 = MINOR(from2);

    unsigned int end1 = start1 + count1;
    unsigned int end2 = start2 + count2;

    if (major1 != major2)
        return false;

    return start1 < end2 && start2 < end1;
}

// 分配一个完全干净的major
int alloc_chrdev_region(dev_t *dev,
                        unsigned int first_minor,
                        unsigned int count,
                        const char *name)
{
    struct chrdev_region *region;
    unsigned int major;
    unsigned long flags;

    if (!dev || !name)
        return -EINVAL;

    if (count == 0)
        return -EINVAL;

    if (first_minor > MINORMASK ||
        count - 1 > MINORMASK - first_minor)
        return -EINVAL;

    region = kzalloc(sizeof(*region));
    if (!region)
        return -ENOMEM;

    region->count = count;
    strncpy(region->name, name, sizeof(region->name));
    INIT_LIST_HEAD(&region->node);

    flags = spin_lock_irqsave(&chrdev_registry.lock);

    for (major = chrdev_registry.dynamic_major_start;
         major >= chrdev_registry.dynamic_major_end;
         major--) {

        if (!chrdev_major_in_use(major)) {
            region->from = MKDEV(major, first_minor);

            list_add_tail(&region->node,
                          &chrdev_registry.regions);

            *dev = region->from;

            spin_unlock_irqrestore(&chrdev_registry.lock, flags);
            return 0;
        }

        if (major == chrdev_registry.dynamic_major_end)
            break;
    }

    spin_unlock_irqrestore(&chrdev_registry.lock, flags);

    kfree(region);
    return -EBUSY;
}

// 在给定的major下分配minor, 不给跨major分配
int register_chrdev_region(dev_t from,
                           unsigned int count,
                           const char *name)
{
    struct chrdev_region *region;
    struct chrdev_region *iter;
    unsigned long flags;

    if (!name)
        return -EINVAL;

    if (!chrdev_range_valid(from, count))
        return -EINVAL;

    /*
     * major 0 通常保留，避免使用无效设备号。
     */
    if (MAJOR(from) == 0)
        return -EINVAL;

    region = kzalloc(sizeof(*region));
    if (!region)
        return -ENOMEM;

    region->from = from;
    region->count = count;
    strncpy(region->name, name, sizeof(region->name));
    INIT_LIST_HEAD(&region->node);

    flags = spin_lock_irqsave(&chrdev_registry.lock);

    list_for_each_entry(iter, &chrdev_registry.regions, node) {
        if (chrdev_region_overlap(region->from,
                                  region->count,
                                  iter->from,
                                  iter->count)) {
            spin_unlock_irqrestore(&chrdev_registry.lock, flags);
            kfree(region);
            return -EBUSY;
        }
    }

    list_add_tail(&region->node, &chrdev_registry.regions);

    spin_unlock_irqrestore(&chrdev_registry.lock, flags);

    return 0;
}

struct cdev *cdev_alloc() {
    struct cdev *dev = kzalloc(sizeof(struct cdev));
    INIT_LIST_HEAD(&dev->list);
    return dev;
}

int cdev_add(struct cdev *cdev, dev_t devnr, int count) {
    struct cdev *iter = NULL;

    list_for_each_entry(iter, &g_cdevs, list) {
        if (strcmp(iter->name, cdev->name) == 0) {
            
            return -EEXIST;
        }
    }

    if (!cdev || !cdev->name)
        return -EINVAL;
    cdev->devnr = devnr;
    cdev->count = count;

    list_add_tail( &g_cdevs,&cdev->list);
    return 0;
}

int cdev_register(const char *name, dev_t devnr, int count, const struct file_operations *fops, void *private) {
    struct cdev *cdev;

    struct cdev *iter = NULL;

    list_for_each_entry(iter, &g_cdevs, list) {
        if (strcmp(iter->name, name) == 0) {
            
            return -EEXIST;
        }
    }

    cdev = cdev_alloc();
    if (!cdev)
        return -ENOMEM;

    cdev->name = strdup(name);
    cdev->devnr = devnr;
    cdev->count = count;
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

struct cdev *cdev_get_by_devnr(dev_t devnr) {
    struct list_head *pos;

    list_for_each(pos, &g_cdevs) {
        struct cdev *cdev = container_of(pos, struct cdev, list);
        if (devnr >= cdev->devnr && devnr < cdev->devnr + cdev->count) {
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
