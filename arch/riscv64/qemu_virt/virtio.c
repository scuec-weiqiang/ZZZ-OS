/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/virtio.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-21 14:21:01
 * @LastEditTime: 2025-05-27 14:33:21
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "virtio.h"
#include "printf.h"
#include "color.h"
#include "page_alloc.h"

volatile virtio_mmio_regs_t *virtio = (volatile virtio_mmio_regs_t *)VIRTIO_MMIO_BASE;

void virtio_blk_init()
{
    printf("magic=%x, version=%x, vendor_id=%x\n", virtio->magic, virtio->version, virtio->vendor_id);
    if(virtio->magic != VIRTIO_MMIO_MAGIC_VALUE ||
       virtio->version != VIRTIO_MMIO_VERSION ||
       virtio->vendor_id != VIRTIO_MMIO_VENDOR_ID)
    {
        panic(RED("virtio_blk_init: virtio device not found!\n"));
    }

    
    // 1. Reset the device.
    virtio->status = 0;
    __sync_synchronize();

    // 2. Set the ACKNOWLEDGE status bit: the guest OS has noticed the device
    virtio->status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;

    __sync_synchronize();
    // 3. Set the DRIVER status bit: the guest OS knows how to drive the device.
    virtio->status |= VIRTIO_CONFIG_S_DRIVER;

    __sync_synchronize();
    virtio->status |= VIRTIO_CONFIG_S_DRIVER_OK;

    __sync_synchronize();
    // 4. Read device feature bits, and write the subset of feature bits understood by the OS and driver to the device.
    uint32_t features = virtio->driver_features;
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    virtio->driver_features = features;

    // 5. Set the FEATURES_OK status bit. The driver MUST NOT accept new feature bits after this step. 
    virtio->status |= VIRTIO_CONFIG_S_FEATURES_OK;

    // 6. Re-read device status to ensure the FEATURES_OK bit is still set: otherwise, the device does not
    //  support our subset of features and the device is unusable
    if(!(virtio->status & VIRTIO_CONFIG_S_FEATURES_OK))
    {
        panic(RED("virtio_blk_init: virtio device does not support features and cannot be used!"));
    }

}


