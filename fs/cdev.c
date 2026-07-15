#include <fs/cdev.h>
#include <os/err.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/list.h>
#include <os/device.h>

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

            list_add_tail(&chrdev_registry.regions,
                          &region->node);

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

    list_add_tail(&chrdev_registry.regions, &region->node);

    spin_unlock_irqrestore(&chrdev_registry.lock, flags);

    return 0;
}

void unregister_chrdev_region(dev_t from, unsigned int count)
{
    struct chrdev_region *region;
    struct chrdev_region *tmp;
    unsigned long flags;

    if (!chrdev_range_valid(from, count))
        return;

    flags = spin_lock_irqsave(&chrdev_registry.lock);
    list_for_each_entry_safe(region, tmp, &chrdev_registry.regions, node) {
        if (region->from == from && region->count == count) {
            list_del(&region->node);
            spin_unlock_irqrestore(&chrdev_registry.lock, flags);
            kfree(region);
            return;
        }
    }
    spin_unlock_irqrestore(&chrdev_registry.lock, flags);
}

struct cdev *cdev_alloc() {
    struct cdev *dev = kzalloc(sizeof(struct cdev));
    if (!dev)
        return NULL;
    INIT_LIST_HEAD(&dev->list);
    return dev;
}

void cdev_init(struct cdev *cdev, const struct file_operations *fops)
{
    if (!cdev)
        return;
    memset(cdev, 0, sizeof(*cdev));
    INIT_LIST_HEAD(&cdev->list);
    cdev->fops = fops;
}

int cdev_add(struct cdev *cdev, dev_t devnr, int count) {
    struct cdev *iter;

    if (!cdev || !cdev->fops || count <= 0)
        return -EINVAL;
    list_for_each_entry(iter, &g_cdevs, list) {
        dev_t end = devnr + count;
        dev_t iter_end = iter->devnr + iter->count;
        if (devnr < iter_end && iter->devnr < end)
            return -EEXIST;
    }
    cdev->devnr = devnr;
    cdev->count = count;

    list_add_tail( &g_cdevs,&cdev->list);
    return 0;
}

void cdev_del(struct cdev *cdev)
{
    if (!cdev)
        return;
    list_del(&cdev->list);
}

int cdev_register(const char *name, dev_t devnr,
                  const struct file_operations *fops, void *private) {
    struct cdev *cdev;
    struct device *dev;
    static struct class misc_class = { .name = "misc" };
    static bool misc_class_ready;
    int ret;

    cdev = cdev_alloc();
    if (!cdev)
        return -ENOMEM;

    cdev->fops = fops;
    cdev->private = private;

    ret = cdev_add(cdev, devnr, 1);
    if (ret) {
        kfree(cdev);
        return ret;
    }

    if (!misc_class_ready) {
        ret = class_register(&misc_class);
        if (ret && ret != -EEXIST) {
            cdev_del(cdev);
            kfree(cdev);
            return ret;
        }
        misc_class_ready = true;
    }

    dev = device_create(&misc_class, NULL, devnr, S_IFCHR | 0600,
                        private, name);
    if (IS_ERR(dev)) {
        ret = PTR_ERR(dev);
        cdev_del(cdev);
        kfree(cdev);
        return ret;
    }
    return 0;
}

struct cdev* cdev_get_by_path(const char *path) {
    struct device *dev;
    struct cdev *cdev;

    if (!path)
        return ERR_PTR(-EINVAL);

    if (strncmp(path, "/dev/", 5) == 0)
        path += 5;

    dev = device_find_by_name(path);
    if (!dev || !S_ISCHR(dev->mode))
        return ERR_PTR(-ENODEV);

    cdev = cdev_get_by_devnr(dev->devt);
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
