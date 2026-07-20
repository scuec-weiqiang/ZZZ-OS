/**
 * @FilePath     : /ZZZ-OS/drivers/video/fb_core.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-17 15:29:39
 * @LastEditTime : 2026-07-17 16:02:36
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/fb.h>
#include <os/string.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/printk.h>

static int fb_open(struct inode *inode, struct file *file)
{
    struct fb_info *info = file->private_data;

    (void)inode;

    if (!info || !info->screen_base)
        return -ENODEV;

    file->private_data = info;
    return 0;
}

static ssize_t fb_write(struct file *file, const char *buf,
                        size_t count, loff_t *ppos)
{
    struct fb_info *info = file->private_data;
    size_t pos = *ppos;

    if (!info || !info->screen_base || !ppos)
        return -ENODEV;

    if (!buf && count)
        return -EINVAL;

    if (pos >= info->size)
        return 0;

    if (count > info->size - pos)
        count = info->size - pos;

    memcpy((u8 *)info->screen_base + pos, buf, count);
    *ppos += count;
    return count;
}

static const struct file_operations fb_fops = {
    .open = fb_open,
    .write = fb_write,
};


static struct class graphics_class = {
    .name = "graphics",
};

static bool graphics_class_ready;

int register_framebuffer(struct fb_info *info, struct device *parent)
{
    int ret;

    if (!info || !info->screen_base || !info->size)
        return -EINVAL;

    if (!graphics_class_ready) {
        ret = class_register(&graphics_class);
        if (ret && ret != -EEXIST)
            return ret;
        graphics_class_ready = true;
    }

    ret = alloc_chrdev_region(&info->devt, 0, 1, "fb");
    if (ret)
        return ret;

    cdev_init(&info->cdev, &fb_fops);
    info->cdev.private = info;

    ret = cdev_add(&info->cdev, info->devt, 1);
    if (ret)
        goto err_region;

    info->dev = device_create(&graphics_class, parent, info->devt,
                              S_IFCHR | 0600, info, "fb0");
    if (IS_ERR(info->dev)) {
        ret = PTR_ERR(info->dev);
        info->dev = NULL;
        goto err_cdev;
    }

    printk("fb: registered /dev/fb0, %ux%u-%u\n",
           info->width, info->height, info->bpp);
    return 0;

err_cdev:
    cdev_del(&info->cdev);
err_region:
    unregister_chrdev_region(info->devt, 1);
    info->devt = 0;
    return ret;
}

void unregister_framebuffer(struct fb_info *info)
{
    if (!info || !info->devt)
        return;

    if (info->dev) {
        device_destroy(&graphics_class, info->devt);
        info->dev = NULL;
    }

    cdev_del(&info->cdev);
    unregister_chrdev_region(info->devt, 1);
    info->devt = 0;
}
