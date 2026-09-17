/**
 * @FilePath: /ZZZ-OS/drivers/virtio.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-21 14:21:01
 * @LastEditTime: 2025-10-29 21:56:16
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

// #include <os/mm.h>

// volatile struct virtio_mmio_regs *virtio = NULL;

// int virtio_blk_init()
// {
//     if(virtio == NULL)
//     {
//         return -1;
//     }
//     if(virtio->magic != VIRTIO_MMIO_MAGIC_VALUE ||
//        virtio->version != VIRTIO_MMIO_VERSION ||
//        virtio->vendor_id != VIRTIO_MMIO_VENDOR_ID)
//     {
//         return -1;
//     }

//     u32 status = 0; 
//     // 1. Reset the device.
//     virtio->status = status;
//     __sync_synchronize();

//     // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device
//     status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
//     virtio->status = status;
//     __sync_synchronize();

//     // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
//     status |= VIRTIO_CONFIG_S_DRIVER;
//     virtio->status = status;

//     __sync_synchronize();

//     // 4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device.
//     virtio->device_features_sel = 0;
//     __sync_synchronize();
//     u32 device_features = virtio->device_features;
//     u32 features = device_features;
//     // 这里设置你想要启用的功能位
//     features &= ~(1 << VIRTIO_BLK_F_RO);
//     features &= ~(1 << VIRTIO_BLK_F_SCSI);
//     features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
//     features &= ~(1 << VIRTIO_BLK_F_MQ);
//     features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
//     features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
//     features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);

//     // 写入驱动支持的特性
//     virtio->driver_features_sel = 0;
//     __sync_synchronize();
//     virtio->driver_features = features;
//     __sync_synchronize();

//     // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step. 
//     status |= VIRTIO_CONFIG_S_FEATURES_OK;
//     virtio->status = status;

//     __sync_synchronize();
//     // 6. Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
//     //  support our subset of features and the device is unusable
//     if(!(virtio->status & VIRTIO_CONFIG_S_FEATURES_OK))
//     {
//         return -1;
//     }

//     return 0;

// }

#include <os/virtio.h>
#include <os/device.h>
#include <os/of.h>
#include <os/spinlock.h>
#include <os/bus.h>
#include <os/err.h>

struct bus_type virtio_bus_type;

struct device virtio_bus = {
	.name	= "virtio",
};

static inline int virtio_id_match(const struct virtio_device *dev,
				  const struct virtio_device_id *id)
{
	if (id->device != dev->id.device && id->device != VIRTIO_DEV_ANY_ID)
		return 0;

	return id->vendor == VIRTIO_DEV_ANY_ID || id->vendor == dev->id.vendor;
}

static int virtio_match(struct device *_dv, const struct device_driver *_dr)
{
	unsigned int i;
	struct virtio_device *dev = dev_to_virtio(_dv);
	const struct virtio_device_id *ids;

	ids = drv_to_virtio((struct device_driver *)_dr)->id_table;
	for (i = 0; ids[i].device; i++)
		if (virtio_id_match(dev, &ids[i]))
			return 1;
	return 0;
}


// SPINLOCK_DEFINE(alloc_id_lock);
// static int alloc_id(struct virtio_device *vdev) {
//     static int id = 0;
//     spin_lock(&alloc_id_lock);
//     id++;
//     spin_unlock(&alloc_id_lock);
//     return id;
// }

int virtio_device_register(struct virtio_device *vdev) 
{
    if (!vdev) {
        return -1;
    }
    return device_register(&vdev->dev);
}

int virtio_device_unregister(struct virtio_device *vdev) 
{
    if (!vdev) {
        return -1;
    }
    device_unregister(&vdev->dev);
    return 0;
}

int virtio_driver_register(struct virtio_driver *vdrv) 
{
    if (!vdrv) {
        return -1;
    }
    vdrv->driver.bus = &virtio_bus_type;
    return driver_register(&vdrv->driver);
}

int virtio_driver_unregister(struct virtio_driver *vdrv) 
{
    if (!vdrv) {
        return -1;
    }
    driver_unregister(&vdrv->driver);
    return 0;
}

static void virtio_add_status(struct virtio_device *vdev, u32 status)
{
    u32 old_status;

    old_status = vdev->config->get_status(vdev);
    vdev->config->set_status(vdev, old_status | status);
}

void virtio_device_ready(struct virtio_device *vdev)
{
    if (!vdev || !vdev->config || !vdev->config->get_status ||
        !vdev->config->set_status)
        return;

    if (!(vdev->config->get_status(vdev) & VIRTIO_CONFIG_S_DRIVER_OK))
        virtio_add_status(vdev, VIRTIO_CONFIG_S_DRIVER_OK);
}

static int virtio_drv_probe(struct device *_dev) 
{
    struct virtio_device *vdev = to_virtio_device(_dev);
    struct virtio_driver *vdrv = to_virtio_driver(_dev->driver);
    const struct virtio_config_ops *config = vdev->config;
    u64 device_features;
    u64 supported_features;
    int ret;

    if (!vdrv->probe || !config || !config->reset ||
        !config->get_status || !config->set_status ||
        !config->get_features || !config->finalize_features)
        return -ENODEV;
    
    config->reset(vdev);
    virtio_add_status(vdev, VIRTIO_CONFIG_S_ACKNOWLEDGE);
    virtio_add_status(vdev, VIRTIO_CONFIG_S_DRIVER);

    // 接收驱动支持的特性
    device_features = config->get_features(vdev);
    supported_features = vdrv->feature_mask |
                         VIRTIO_FEATURE_BIT(VIRTIO_F_VERSION_1);
    vdev->features = device_features & supported_features;

    // 这里只接受现代virtio设备
    if (!(vdev->features & VIRTIO_FEATURE_BIT(VIRTIO_F_VERSION_1))) {
        ret = -ENODEV;
        goto failed;
    }
    // 写入特性
    ret = config->finalize_features(vdev, vdev->features);
    if (ret)
        goto failed;
    // 检查设备是否接受了特性
    virtio_add_status(vdev, VIRTIO_CONFIG_S_FEATURES_OK);
    if (!(config->get_status(vdev) & VIRTIO_CONFIG_S_FEATURES_OK)) {
        ret = -ENODEV;
        goto failed;
    }

    ret = vdrv->probe(vdev);
    if (ret)
        goto failed;

    virtio_device_ready(vdev);
    return 0;

failed:
    virtio_add_status(vdev, VIRTIO_CONFIG_S_FAILED);
    return ret;
}

static int virtio_drv_remove(struct device *_dev) 
{
    struct virtio_device *dev = to_virtio_device(_dev);
	struct virtio_driver *drv = to_virtio_driver(_dev->driver);
	
	int ret = 0;
    if (drv->remove) {
        ret = drv->remove(dev);
    }
	
	return ret;
}

struct bus_type virtio_bus_type = {
    .name = "virtio",
    .match = virtio_match,
    .probe = virtio_drv_probe,
    .remove = virtio_drv_remove,
    .devices = LIST_HEAD_INIT(virtio_bus_type.devices),
    .drivers = LIST_HEAD_INIT(virtio_bus_type.drivers),
    .lock = SPINLOCK_INIT
};

int virtio_bus_init() 
{
    device_register(&virtio_bus);
    return bus_register(&virtio_bus_type);
}

core_initcall(virtio_bus_init);
