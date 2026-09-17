/**
 * @FilePath     : /ZZZ-OS/drivers/virtio/virtio_mmio.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-13 22:52:24
 * @LastEditTime : 2026-09-16 20:48:29
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <os/virtio.h>
#include <os/device.h>
#include <os/platform_device.h>
#include <os/kmalloc.h>
#include <os/io.h>
#include <os/err.h>
#include <os/kva.h>

static u64  virtio_mmio_get_features(struct virtio_device *vdev) 
{
    struct virtio_mmio_device *vmdev =
        to_virtio_mmio_device(vdev);
    volatile struct virtio_mmio_regs *base = vmdev->base;
    u32 low, high;

    writel(0, &base->device_features_sel);
    low = readl(&base->device_features);

    writel(1, &base->device_features_sel);
    high = readl(&base->device_features);

    return (u64)low | ((u64)high << 32);
}

static int virtio_mmio_finalize_features(struct virtio_device *vdev, u64 features)
{
    struct virtio_mmio_device *vmdev =
        to_virtio_mmio_device(vdev);
    volatile struct virtio_mmio_regs *base = vmdev->base;

    writel(0, &base->driver_features_sel);
    writel((u32)features, &base->driver_features);

    writel(1, &base->driver_features_sel);
    writel((u32)(features >> 32), &base->driver_features);

    return 0;
}

static u32 virtio_mmio_get_status(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vmdev =
        to_virtio_mmio_device(vdev);

    return readl(&vmdev->base->status);
}

static void virtio_mmio_set_status(struct virtio_device *vdev, u32 status)
{
    struct virtio_mmio_device *vmdev =
        to_virtio_mmio_device(vdev);

    writel(status, &vmdev->base->status);
}

static void virtio_mmio_reset(struct virtio_device *vdev)
{
    struct virtio_mmio_device *vmdev =
        to_virtio_mmio_device(vdev);

    writel(0, &vmdev->base->status);

    while (readl(&vmdev->base->status) != 0);
}

// 把virtqueue的desc、avail、used的物理地址写入设备寄存器，并设置队列可用
static int virtio_mmio_setup_vq(struct virtio_device *vdev, struct virtqueue *vq, unsigned int index)
{
    struct virtio_mmio_device *vmdev = to_virtio_mmio_device(vdev);
    volatile struct virtio_mmio_regs *base = vmdev->base;
    u32 max;
    u64 pa;

    writel(index, &base->queue_sel);

    // 检查设备是否提供这条队列
    max = readl(&base->queue_num_max);
    if (max == 0)
        return -ENOENT;

    // 队列不能已经被其他驱动启用
    if (readl(&base->queue_ready))
        return -EBUSY;

    // 选择实际队列长度 
    if (vq->num > max)
        vq->num = max;

    writel(vq->num, &base->queue_num);

    // 写descriptor table物理地址
    pa = KERNEL_PA(vq->desc);
    writel((u32)pa, &base->queue_desc_low);
    writel((u32)(pa >> 32), &base->queue_desc_high);

    // 写available ring物理地址
    pa = KERNEL_PA(vq->avail);
    writel((u32)pa, &base->queue_avail_low);
    writel((u32)(pa >> 32), &base->queue_avail_high);

    // 写used ring物理地址
    pa = KERNEL_PA(vq->used);
    writel((u32)pa, &base->queue_used_low);
    writel((u32)(pa >> 32), &base->queue_used_high);

    // 队列正式可用
    writel(1, &base->queue_ready);
    
    return 0;
}

static void virtio_mmio_notify(struct virtqueue *vq)
{
    struct virtio_mmio_device *vmdev = to_virtio_mmio_device(vq->vdev);
    writel(vq->index, &vmdev->base->queue_notify);
}

static void virtio_mmio_get_config(struct virtio_device *vdev,
                                   unsigned int offset,
                                   void *buf,
                                   unsigned int len)
{
    struct virtio_mmio_device *vmdev = to_virtio_mmio_device(vdev);
    volatile u8 *config = (volatile u8 *)vmdev->base->config;
    u8 *dst = buf;
    u32 before;
    u32 after;

    if (!buf || !len)
        return;

    do {
        before = readl(&vmdev->base->config_generation);
        for (unsigned int i = 0; i < len; i++) {
            dst[i] = readb(config + offset + i);
        }
        // 防止读取过程中配置发生变化
        after = readl(&vmdev->base->config_generation);
    } while (before != after);
}

static const struct virtio_config_ops virtio_mmio_config_ops = {
    .get_features      = virtio_mmio_get_features,
    .finalize_features = virtio_mmio_finalize_features,
    .get_status        = virtio_mmio_get_status,
    .set_status        = virtio_mmio_set_status,
    .reset             = virtio_mmio_reset,
    .setup_vq          = virtio_mmio_setup_vq,
    .notify            = virtio_mmio_notify,
    .get_config        = virtio_mmio_get_config,
};

static int virtio_mmio_probe(struct platform_device *pdev) 
{
	struct virtio_mmio_device *vmdev;
    struct virtio_mmio_regs *base = 
            (struct virtio_mmio_regs*)platform_ioremap_resource(pdev, 0);
	
    if (base->magic != VIRTIO_MMIO_MAGIC_VALUE 
        || base->version != VIRTIO_MMIO_VERSION
        || base->device_id == 0) {
        return -ENODEV;
    }

    vmdev = kzalloc(sizeof(*vmdev));
    device_initialize(&vmdev->vdev.dev);
    INIT_LIST_HEAD(&vmdev->vdev.vqs);

	vmdev->pdev = pdev;
	vmdev->base = base;
	vmdev->irq = platform_get_irq(pdev, 0);

    vmdev->vdev.dev.name = pdev->dev.name;
    vmdev->vdev.dev.parent = &pdev->dev;
    vmdev->vdev.dev.bus = &virtio_bus_type;

    vmdev->vdev.id.device = readl(&base->device_id);
    vmdev->vdev.id.vendor = readl(&base->vendor_id);
    vmdev->vdev.config = &virtio_mmio_config_ops;

    INIT_LIST_HEAD(&vmdev->vdev.vqs);
    vmdev->version = base->version;
    
    /*注册virtio设备，挂到virtio设备总线上*/
    int ret = virtio_device_register(&vmdev->vdev);
    if (ret) {
        kfree(vmdev);
        return ret;
    }
    dev_set_drvdata(&pdev->dev, vmdev);
    printk("virtio-mmio: found device=%d vendor=%xu\n",
           vmdev->vdev.id.device, vmdev->vdev.id.vendor);
    return ret;
}
  

static int virtio_mmio_remove(struct platform_device *pdev) 
{
    return 0;
}


static const struct of_device_id virtio_mmio_match[] = {
    { .compatible = "virtio,mmio" },
    {}
};

static struct platform_driver virtio_mmio_driver = {
    .name = "virtio-mmio",
    .probe = virtio_mmio_probe,
    .remove = virtio_mmio_remove,
    .driver = {
        .name = "virtio-mmio",
        .of_match_table = virtio_mmio_match,
    },
};

module_platform_driver(virtio_mmio_driver);
