/**
 * @FilePath: /vboot/virtio.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-20 20:12:02
 * @LastEditTime: 2025-09-17 21:03:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef VIRTIO_H
#define VIRTIO_H

#include "types.h"

#define VIRTIO_MMIO_BASE 0x10001000

#define LEGACY 0

// Virtio MMIO 寄存器布局（参考 Virtio 规范 1.1）
struct virtio_mmio_regs
{
    /*0x000*/ volatile u32 magic;               // 魔数 "virt" (0x74726976)
    /*0x004*/ volatile u32 version;             // 版本 (应为 2)
    /*0x008*/ volatile u32 device_id;           // 设备 ID（块设备为 2）
    /*0x00c*/ volatile u32 vendor_id;           // 厂商 ID（忽略）
    /*0x010*/ volatile u32 device_features;     // 设备支持的特性
    /*0x014*/ volatile u32 device_features_sel; // 设备特性选择
    /*0x018*/ volatile u8 reserved_1[8];
    /*0x020*/ volatile u32 driver_features;     // 驱动启用的特性
    /*0x024*/ volatile u32 driver_features_sel; // 驱动特性选择
    /*0x028*/ volatile u8 reserved_2[8];
    /*0x030*/ volatile u32 queue_sel;     // 队列选择
    /*0x034*/ volatile u32 queue_num_max; // 队列最大长度
    /*0x038*/ volatile u32 queue_num;     // 队列实际长度
    /*0x03c*/ volatile u8 reserved_3[8];
    /*0x044*/ volatile u32 queue_ready; // 队列对齐要求
    /*0x048*/ volatile u8 reserved_4[8];
    /*0x050*/ volatile u32 queue_notify; // 队列通知寄存器
    /*0x054*/ volatile u8 reserved_5[12];
    /*0x060*/ volatile u32 interrupt_status; // 中断状态
    /*0x064*/ volatile u32 interrupt_ack;    // 中断确认
    /*0x068*/ volatile u8 reserved_6[8];
    /*0x070*/ volatile u32 status; // 设备状态
    /*0x074*/ volatile u8 reserved_7[12];
    /*0x080*/ volatile u32 queue_desc_low;
    /*0x084*/ volatile u32 queue_desc_high;
    /*0x088*/ volatile u8 reserved_8[8];
    /*0x090*/ volatile u32 queue_avail_low;
    /*0x094*/ volatile u32 queue_avail_high;
    /*0x098*/ volatile u8 reserved_9[8];
    /*0x0a0*/ volatile u32 queue_used_low;
    /*0x0a4*/ volatile u32 queue_used_high;
    /*0x0a8*/ volatile u8 reserved_10[84];
    /*0x0fc*/ volatile u32 config_generation; // 配置生成号（忽略）
    /*0x100*/ volatile u32 config[0];         // 设备配置空间（块设备为 struct virtio_blk_config）
};

extern volatile struct virtio_mmio_regs *virtio;

// // 块设备配置结构（位于 config 字段）
struct virtio_blk_config
{
    u64 capacity; // 磁盘容量（扇区数）
    u32 size_max; // 最大请求大小
    u32 seg_max;  // 最大段数

    // ... 其他字段（可忽略）
    struct virtio_blk_geometry
    {
        u16 cylinders;
        u8 heads;
        u8 sectors;
    } geometry;

    u32 blk_size;

    struct virtio_blk_topology
    {
        // # of logical blocks per physical block (log2)
        u8 physical_block_exp;
        // offset of first aligned logical block
        u8 alignment_offset;
        // suggested minimum I/O size in blocks
        u16 min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        u32 opt_io_size;
    } topology;

    u8 reserved;
};

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976 // 0x74726976
#define VIRTIO_MMIO_VERSION 0x002          // version; should be 2
#define VIRTIO_MMIO_DEVICE_ID 0x002        // device type; 1 is net, 2 is disk
#define VIRTIO_MMIO_VENDOR_ID 0x554d4551   // 0x554d4551

// status register bits, from qemu virtio_config.h
#define VIRTIO_CONFIG_S_ACKNOWLEDGE 1
#define VIRTIO_CONFIG_S_DRIVER 2
#define VIRTIO_CONFIG_S_DRIVER_OK 4
#define VIRTIO_CONFIG_S_FEATURES_OK 8

// device feature bits
#define VIRTIO_BLK_F_RO 5   /* Disk is read-only */
#define VIRTIO_BLK_F_SCSI 7 /* Supports scsi command passthru */
#define VIRTIO_BLK_F_FLUSH 9
#define VIRTIO_BLK_F_CONFIG_WCE 11 /* Writeback mode available in config */
#define VIRTIO_BLK_F_MQ 12         /* support more than one vq */
#define VIRTIO_F_ANY_LAYOUT 27
#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

// this many virtio descriptors.
// must be a power of two.
#define QUEUE_NUM 8

struct virtq_desc
{
    /* Address (guest-physical). */
    u64 addr;
    /* Length. */
    u32 len;
/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
    /* The flags as indicated above. */
    u16 flags;
    /* Next field if flags & NEXT */
    u16 next;
};

struct virtq_avail
{
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    u16 flags;
    u16 idx;
    u16 ring[QUEUE_NUM];
    u16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
};

/* u32 is used here for ids for padding reasons. */
struct virtq_used_elem
{
    /* Index of start of used descriptor chain. */
    u32 id;
    /* Total length of the descriptor chain which was used (written to) */
    u32 len;
};

struct virtq_used
{
#define VIRTQ_USED_F_NO_NOTIFY 1
    u16 flags;
    u16 idx;
    struct virtq_used_elem ring[QUEUE_NUM];
    u16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */
};

struct virtq
{
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
};

#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk
struct virtio_blk_req
{
    u32 type;
    u32 reserved;
    u64 sector;
} __attribute__((packed));

extern int virtio_blk_init();

#endif