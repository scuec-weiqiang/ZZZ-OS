#include <os/devnode.h>
#include <os/device.h>
#include <fs/dcache.h>
#include <fs/fs.h>
#include <os/errno.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/printk.h>
#include <os/string.h>

struct devtmpfs_entry {
    struct device *dev;
    bool published;
    struct list_head node;
};

static LIST_HEAD(devtmpfs_entries);
static const char *devtmpfs_path;
static bool devtmpfs_ready;

static int devtmpfs_publish(struct devtmpfs_entry *entry)
{
    struct device *dev = entry->dev;
    struct dentry *dentry;
    char *path;
    size_t base_len;
    int ret = 0;

    if (!devtmpfs_ready || entry->published)
        return 0;

    base_len = strlen(devtmpfs_path);
    path = kmalloc(base_len + 1 + strlen(dev->name) + 1);
    if (!path)
        return -ENOMEM;

    strcpy(path, devtmpfs_path);
    if (base_len && path[base_len - 1] != '/') {
        path[base_len++] = '/';
        path[base_len] = '\0';
    }
    strcpy(path + base_len, dev->name);

    dentry = vfs_mknod(path, dev->mode, dev->devt);
    if (IS_ERR(dentry))
        ret = PTR_ERR(dentry);
    else if (!dentry)
        ret = -EIO;
    else if (dentry)
        dput(dentry);

    if (!ret) {
        entry->published = true;
        printk("devtmpfs: created %s (%u:%u)\n", path,
               MAJOR(dev->devt), MINOR(dev->devt));
    } else {
        printk("devtmpfs: failed to create %s: %d\n", path, ret);
    }
    kfree(path);
    return ret;
}

int devtmpfs_create_node(struct device *dev)
{
    struct devtmpfs_entry *entry, *iter;

    if (!dev || !dev->name || !dev->devt ||
        (!S_ISCHR(dev->mode) && !S_ISBLK(dev->mode)))
        return -EINVAL;

    list_for_each_entry(iter, &devtmpfs_entries, node) {
        if (iter->dev->devt == dev->devt ||
            strcmp(iter->dev->name, dev->name) == 0)
            return -EEXIST;
    }

    entry = kzalloc(sizeof(*entry));
    if (!entry)
        return -ENOMEM;
    entry->dev = dev;
    INIT_LIST_HEAD(&entry->node);
    list_add_tail(&devtmpfs_entries, &entry->node);

    if (devtmpfs_publish(entry)) {
        list_del(&entry->node);
        kfree(entry);
        return -EIO;
    }
    return 0;
}

void devtmpfs_remove_node(struct device *dev)
{
    struct devtmpfs_entry *entry, *next;

    list_for_each_entry_safe(entry, next, &devtmpfs_entries, node) {
        char *path;
        size_t base_len;

        if (entry->dev != dev)
            continue;
        if (entry->published) {
            base_len = strlen(devtmpfs_path);
            path = kmalloc(base_len + 1 + strlen(dev->name) + 1);
            if (path) {
                strcpy(path, devtmpfs_path);
                if (base_len && path[base_len - 1] != '/') {
                    path[base_len++] = '/';
                    path[base_len] = '\0';
                }
                strcpy(path + base_len, dev->name);
                vfs_unlink(path);
                kfree(path);
            }
        }
        list_del(&entry->node);
        kfree(entry);
        return;
    }
}

int devtmpfs_mount(const char *path)
{
    struct devtmpfs_entry *entry;

    if (!path)
        return -EINVAL;
    if (!devtmpfs_path) {
        devtmpfs_path = strdup(path);
        if (!devtmpfs_path)
            return -ENOMEM;
    }
    devtmpfs_ready = true;

    list_for_each_entry(entry, &devtmpfs_entries, node) {
        int ret = devtmpfs_publish(entry);
        if (ret)
            return ret;
    }
    return 0;
}
