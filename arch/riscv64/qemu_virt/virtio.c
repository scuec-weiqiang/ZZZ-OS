#include "virtio.h"
#include "printf.h"
#include "color.h"

void virtio_disk_init()
{
    if(virtio->magic != VIRTIO_MMIO_MAGIC_VALUE ||
       virtio->version != VIRTIO_MMIO_VERSION ||
       virtio->vendor_id != VIRTIO_MMIO_VENDOR_ID)
    {
        panic(RED("virtio_blk_init: virtio device not found"));
    }

    // 1. Reset the device.
    virtio->status = 0;
    
    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device
    virtio->status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;

    // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
    virtio->status |= VIRTIO_CONFIG_S_DRIVER;

    // 4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device.
    uint32_t features = virtio->device_features;
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    virtio->device_features = features;

    // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step. 
    virtio->status |= VIRTIO_CONFIG_S_FEATURES_OK;

    // 6. Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
    //  support our subset of features and the device is unusable
    if(!(virtio->status & VIRTIO_CONFIG_S_FEATURES_OK))
    {
        panic(RED("virtio_blk_init: virtio device does not support features and cannot be used!"));
    }

    virtio->status |= VIRTIO_CONFIG_S_DRIVER_OK;
}