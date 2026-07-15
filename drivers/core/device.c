#include <os/device.h>
#include <os/bus.h>
#include <os/devnode.h>
#include <os/errno.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/string.h>

static LIST_HEAD(device_list);
static LIST_HEAD(class_list);

int class_register(struct class *class)
{
    if (!class || !class->name)
        return -EINVAL;
    if (class->registered)
        return -EEXIST;

    INIT_LIST_HEAD(&class->devices);
    INIT_LIST_HEAD(&class->node);
    list_add_tail(&class_list, &class->node);
    class->registered = true;
    return 0;
}

void class_unregister(struct class *class)
{
    if (!class || !class->registered)
        return;
    if (!list_empty(&class->devices))
        return;

    list_del(&class->node);
    class->registered = false;
}

int device_register(struct device *dev)
{
    int ret;

    if (!dev || !dev->name)
        return -EINVAL;
    if (dev->registered)
        return -EEXIST;
    if (dev->class && !dev->class->registered)
        return -EINVAL;

    INIT_LIST_HEAD(&dev->bus_node);
    INIT_LIST_HEAD(&dev->class_node);
    INIT_LIST_HEAD(&dev->global_node);

    if (dev->bus) {
        ret = bus_add_device(dev);
        if (ret)
            return ret;

        ret = device_attach(dev);
        if (ret) {
            bus_remove_device(dev);
            return ret;
        }
    }

    if (dev->class)
        list_add_tail(&dev->class->devices, &dev->class_node);
    list_add_tail(&device_list, &dev->global_node);
    dev->registered = true;

    if (dev->devt) {
        ret = devtmpfs_create_node(dev);
        if (ret) {
            device_unregister(dev);
            return ret;
        }
    }

    return 0;
}

int device_unregister(struct device *dev)
{
    if (!dev || !dev->registered)
        return -EINVAL;

    if (dev->devt)
        devtmpfs_remove_node(dev);
    list_del(&dev->global_node);
    if (dev->class)
        list_del(&dev->class_node);
    if (dev->bus)
        bus_remove_device(dev);
    dev->registered = false;
    return 0;
}

struct device *device_create(struct class *class, struct device *parent,
                             dev_t devt, mode_t mode, void *data,
                             const char *name)
{
    struct device *dev;
    int ret;

    if (!class || !name || !devt)
        return ERR_PTR(-EINVAL);

    dev = kzalloc(sizeof(*dev));
    if (!dev)
        return ERR_PTR(-ENOMEM);
    dev->name = strdup(name);
    if (!dev->name) {
        kfree(dev);
        return ERR_PTR(-ENOMEM);
    }

    dev->class = class;
    dev->parent = parent;
    dev->devt = devt;
    dev->mode = mode;
    dev->driver_data = data;

    ret = device_register(dev);
    if (ret) {
        kfree((void *)dev->name);
        kfree(dev);
        return ERR_PTR(ret);
    }
    return dev;
}

void device_destroy(struct class *class, dev_t devt)
{
    struct device *dev, *next;

    if (!class)
        return;
    list_for_each_entry_safe(dev, next, &class->devices, class_node) {
        if (dev->devt != devt)
            continue;
        device_unregister(dev);
        kfree((void *)dev->name);
        kfree(dev);
        return;
    }
}

struct device *device_find_by_devt(dev_t devt)
{
    struct device *dev;

    list_for_each_entry(dev, &device_list, global_node) {
        if (dev->devt == devt)
            return dev;
    }
    return NULL;
}

struct device *device_find_by_name(const char *name)
{
    struct device *dev;

    if (!name)
        return NULL;
    list_for_each_entry(dev, &device_list, global_node) {
        if (strcmp(dev->name, name) == 0)
            return dev;
    }
    return NULL;
}
