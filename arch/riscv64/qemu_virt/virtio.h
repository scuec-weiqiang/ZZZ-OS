#ifndef _VIRTIO_H
#define _VIRTIO_H

#include "types.h"

// Virtio MMIO 寄存器布局（参考 Virtio 规范 1.1）
struct virtio_mmio_regs {
    uint32_t magic;         // 魔数 "virt" (0x74726976)
    uint32_t version;       // 版本 (应为 2)
    uint32_t device_id;     // 设备 ID（块设备为 2）
    uint32_t vendor_id;     // 厂商 ID（忽略）
    uint32_t device_features; // 设备支持的特性
    uint32_t device_features_sel; // 设备特性选择
    uint32_t driver_features;     // 驱动启用的特性
    uint32_t driver_features_sel; // 驱动特性选择
    uint32_t queue_sel;     // 队列选择
    uint32_t queue_num_max; // 队列最大长度
    uint32_t queue_num;     // 队列实际长度
    uint32_t queue_align;   // 队列对齐要求
    uint32_t queue_pfn;     // 队列物理页号
    uint32_t queue_notify;  // 队列通知寄存器
    uint32_t interrupt_status; // 中断状态
    uint32_t interrupt_ack; // 中断确认
    uint32_t status;        // 设备状态
    uint32_t config[0];     // 设备配置空间（块设备为 struct virtio_blk_config）
}virtio_mmio_regs_t;

// 块设备配置结构（位于 config 字段）
struct virtio_blk_config {
    uint64_t capacity;      // 磁盘容量（扇区数）
    uint32_t size_max;      // 最大请求大小
    uint32_t seg_max;       // 最大段数
    // ... 其他字段（可忽略）
}virtio_blk_config_t;

#endif