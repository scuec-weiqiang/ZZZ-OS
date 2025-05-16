#include "virtio.h"

// virtio.c
#include "virtio.h"

#define VIRTIO_BLK_BASE 0x10001000 // Virtio-blk 设备基地址

volatile struct virtio_mmio_regs *regs = (struct virtio_mmio_regs*)VIRTIO_BLK_BASE;

void virtio_blk_init() {
    // 1. 检查魔数和版本
    if (regs->magic != 0x74726976 || regs->version != 2) {
        panic("Invalid Virtio device");
    }

    // 2. 重置设备
    regs->status = 0;

    // 3. 协商特性（这里启用 VIRTIO_BLK_F_RO 只读特性）
    regs->device_features_sel = 0;
    uint32_t features = regs->device_features;
    if (!(features & (1 << 5))) { // 检查是否支持 VIRTIO_BLK_F_RO
        panic("Virtio-blk does not support read-only");
    }
    regs->driver_features = (1 << 5); // 启用 VIRTIO_BLK_F_RO
    regs->driver_features_sel = 0;

    // 4. 初始化队列（选择队列0）
    regs->queue_sel = 0;
    if (regs->queue_num_max < 1) {
        panic("Queue too small");
    }
    regs->queue_num = 1; // 队列长度为1
    regs->queue_align = 4096; // 对齐要求（通常为页大小）
    regs->queue_pfn = (uint32_t)(my_dma_buffer >> 12); // 队列物理地址（需 DMA 内存）

    // 5. 完成初始化
    regs->status |= 0x04; // ACKNOWLEDGE
    regs->status |= 0x02; // DRIVER
    regs->status |= 0x01; // DRIVER_OK
}

