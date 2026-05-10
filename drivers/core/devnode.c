#include <os/devnode.h>
#include <fs/dcache.h>
#include <fs/fs.h>
#include <os/list.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/err.h>
#include <os/printk.h>

static LIST_HEAD(g_devnodes);
static const char *g_devtmpfs_path;
static bool g_devtmpfs_ready;

static int devtmpfs_publish(struct devnode *node) {
    struct dentry *dentry = NULL;
    char *path = NULL;
    int ret = 0;
    size_t base_len = 0;
    size_t name_len = 0;
    u16 mode = 0;

    if (!g_devtmpfs_ready || g_devtmpfs_path == NULL || node == NULL || node->name == NULL) {
        return 0;
    }

    base_len = strlen(g_devtmpfs_path);
    name_len = strlen(node->name);
    path = kmalloc(base_len + 1 + name_len + 1);
    if (path == NULL) {
        return -ENOMEM;
    }

    strcpy(path, g_devtmpfs_path);
    if (base_len > 0 && path[base_len - 1] != '/') {
        path[base_len] = '/';
        path[base_len + 1] = '\0';
    }
    strcpy(path + strlen(path), node->name);

    if (node->type == DEV_CHAR) {
        mode = S_IFCHR | 0600;
    } else if (node->type == DEV_BLOCK) {
        mode = S_IFBLK | 0600;
    } else {
        ret = -EINVAL;
        goto out;
    }

    dentry = vfs_mknod(path, mode, node->devt);
    if (dentry != NULL) {
        dput(dentry);
    }

out:
    kfree(path);
    return ret;
}

int devnode_register(const char *name, int type, dev_t devt,const struct file_operations *fops, void *private) {
    struct devnode *node;
    struct list_head *pos;

    list_for_each(pos, &g_devnodes) {
        struct devnode *iter = list_entry(pos, struct devnode, list);

        if (iter->devt == devt || strcmp(iter->name, name) == 0) {
            return -EEXIST;
        }
    }

    node = kzalloc(sizeof(*node));
    if (!node)
        return -ENOMEM;

    node->name = strdup(name);
    node->type = type;
    node->devt = devt;
    node->fops = fops;
    node->private = private;

    INIT_LIST_HEAD(&node->list);

    list_add_tail(&g_devnodes, &node->list);

    if (g_devtmpfs_ready) {
        int ret = devtmpfs_publish(node);
        if (ret < 0) {
            printk("devtmpfs: publish %s failed: %d\n", node->name, ret);
        }
    }

    return 0;
}

struct devnode *devnode_lookup_by_name(const char *name) {
    struct list_head *pos;

    list_for_each(pos, &g_devnodes) {
        struct devnode *node = list_entry(pos, struct devnode, list);
        if (strcmp(node->name, name) == 0)
            return node;
    }

    return NULL;
}

struct devnode *devnode_lookup_by_devnr(dev_t devnr) {
    struct list_head *pos;

    list_for_each(pos, &g_devnodes) {
        struct devnode *node = list_entry(pos, struct devnode, list);
        if (node->devt == devnr)
            return node;
    }

    return NULL;
}

int devtmpfs_mount(const char *path) {
    struct list_head *pos;

    if (path == NULL) {
        return -EINVAL;
    }

    if (g_devtmpfs_path == NULL) {
        g_devtmpfs_path = strdup(path);
        if (g_devtmpfs_path == NULL) {
            return -ENOMEM;
        }
    }

    g_devtmpfs_ready = true;

    list_for_each(pos, &g_devnodes) {
        struct devnode *node = list_entry(pos, struct devnode, list);
        int ret = devtmpfs_publish(node);
        
        if (ret < 0) {
            printk("devtmpfs: publish existing node %s failed: %d\n", node->name, ret);
        }
    }

    return 0;
}
