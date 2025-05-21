/**
 * @FilePath: /ZZZ/arch/riscv64/qemu_virt/virtio.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-20 20:12:02
 * @LastEditTime: 2025-05-20 23:54:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef VIRTIO_H
#define VIRTIO_H

#include "types.h"

#define VIRTIO_MMIO_BASE 0x10001000

// Virtio MMIO 寄存器布局（参考 Virtio 规范 1.1）
typedef struct virtio_mmio_regs {
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
    uint32_t queue_desc_low;
    uint32_t queue_desc_high;
    uint32_t queue_driver_low;
    uint32_t queue_dirver_high;
    uint32_t queue_device_low;
    uint32_t queue_device_high;
    uint32_t config_generation; // 配置生成号（忽略） 
    uint32_t config[0];     // 设备配置空间（块设备为 struct virtio_blk_config）
}virtio_mmio_regs_t;

volatile virtio_mmio_regs_t *virtio = (volatile virtio_mmio_regs_t *)VIRTIO_MMIO_BASE;

// 块设备配置结构（位于 config 字段）
typedef struct virtio_blk_config {
    uint64_t capacity;      // 磁盘容量（扇区数）
    uint32_t size_max;      // 最大请求大小
    uint32_t seg_max;       // 最大段数
    // ... 其他字段（可忽略）
}virtio_blk_config_t;

#define VIRTIO_MMIO_MAGIC_VALUE		0x74726976 // 0x74726976
#define VIRTIO_MMIO_VERSION		    0x002 // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID		0x002 // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID		0x554d4551 // 0x554d4551

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
#define VIRTIO_CONFIG_S_DRIVER		2
#define VIRTIO_CONFIG_S_DRIVER_OK	4
#define VIRTIO_CONFIG_S_FEATURES_OK	8

// device feature bits
#define VIRTIO_BLK_F_RO              5	/* Disk is read-only */
#define VIRTIO_BLK_F_SCSI            7	/* Supports scsi command passthru */
#define VIRTIO_BLK_F_CONFIG_WCE     11	/* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ             12	/* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT         27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX     29

// this many virtio descriptors.
// must be a power of two.
#define QUEUE_SIZE 8

typedef struct virtq_desc
{
    /* Address (guest-physical). */
    uint64_t addr;
    /* Length. */
    uint32_t len;
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
    /* The flags as indicated above. */
    uint16_t flags;
    /* Next field if flags & NEXT */
    uint16_t next;
}virtq_desc_t;

typedef struct virtq_avail
{
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QUEUE_SIZE];
    uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
}virtq_avail_t;

typedef struct virtq_used
{
#define VIRTQ_USED_F_NO_NOTIFY 1
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[QUEUE_SIZE];
    uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
}virtq_used_t;

/* uint32_t is used here for ids for padding reasons. */
typedef struct virtq_used_elem
{
    /* Index of start of used descriptor chain. */
    uint32_t id;
    /* Total length of the descriptor chain which was used (written to) */
    uint32_t len;
}virtq_used_elem_t;

#endif