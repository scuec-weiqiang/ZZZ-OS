/**
 * @FilePath: /vboot/drivers/virtio.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-20 20:12:02
 * @LastEditTime: 2025-09-17 21:03:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef VIRTIO_H
#define VIRTIO_H

#include "os/types.h"

#define VIRTIO_MMIO_BASE 0x10001000

#define LEGACY 0

// Virtio MMIO 寄存器布局（参考 Virtio 规范 1.1）
struct virtio_mmio_regs
{
    /*0x000*/ volatile uint32_t magic;               // 魔数 "virt" (0x74726976)
    /*0x004*/ volatile uint32_t version;             // 版本 (应为 2)
    /*0x008*/ volatile uint32_t device_id;           // 设备 ID（块设备为 2）
    /*0x00c*/ volatile uint32_t vendor_id;           // 厂商 ID（忽略）
    /*0x010*/ volatile uint32_t device_features;     // 设备支持的特性
    /*0x014*/ volatile uint32_t device_features_sel; // 设备特性选择
    /*0x018*/ volatile uint8_t reserved_1[8];
    /*0x020*/ volatile uint32_t driver_features;     // 驱动启用的特性
    /*0x024*/ volatile uint32_t driver_features_sel; // 驱动特性选择
    /*0x028*/ volatile uint8_t reserved_2[8];
    /*0x030*/ volatile uint32_t queue_sel;     // 队列选择
    /*0x034*/ volatile uint32_t queue_num_max; // 队列最大长度
    /*0x038*/ volatile uint32_t queue_num;     // 队列实际长度
    /*0x03c*/ volatile uint8_t reserved_3[8];
    /*0x044*/ volatile uint32_t queue_ready; // 队列对齐要求
    /*0x048*/ volatile uint8_t reserved_4[8];
    /*0x050*/ volatile uint32_t queue_notify; // 队列通知寄存器
    /*0x054*/ volatile uint8_t reserved_5[12];
    /*0x060*/ volatile uint32_t interrupt_status; // 中断状态
    /*0x064*/ volatile uint32_t interrupt_ack;    // 中断确认
    /*0x068*/ volatile uint8_t reserved_6[8];
    /*0x070*/ volatile uint32_t status; // 设备状态
    /*0x074*/ volatile uint8_t reserved_7[12];
    /*0x080*/ volatile uint32_t queue_desc_low;
    /*0x084*/ volatile uint32_t queue_desc_high;
    /*0x088*/ volatile uint8_t reserved_8[8];
    /*0x090*/ volatile uint32_t queue_avail_low;
    /*0x094*/ volatile uint32_t queue_avail_high;
    /*0x098*/ volatile uint8_t reserved_9[8];
    /*0x0a0*/ volatile uint32_t queue_used_low;
    /*0x0a4*/ volatile uint32_t queue_used_high;
    /*0x0a8*/ volatile uint8_t reserved_10[84];
    /*0x0fc*/ volatile uint32_t config_generation; // 配置生成号（忽略）
    /*0x100*/ volatile uint32_t config[0];         // 设备配置空间（块设备为 struct virtio_blk_config）
};

extern volatile struct virtio_mmio_regs *virtio;

// // 块设备配置结构（位于 config 字段）
struct virtio_blk_config
{
    uint64_t capacity; // 磁盘容量（扇区数）
    uint32_t size_max; // 最大请求大小
    uint32_t seg_max;  // 最大段数

    // ... 其他字段（可忽略）
    struct virtio_blk_geometry
    {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;

    uint32_t blk_size;

    struct virtio_blk_topology
    {
        // # of logical blocks per physical block (log2)
        uint8_t physical_block_exp;
        // offset of first aligned logical block
        uint8_t alignment_offset;
        // suggested minimum I/O size in blocks
        uint16_t min_io_size;
        // optimal (suggested maximum) I/O size in blocks
        uint32_t opt_io_size;
    } topology;

    uint8_t reserved;
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
};

struct virtq_avail
{
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QUEUE_NUM];
    uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
};

/* uint32_t is used here for ids for padding reasons. */
struct virtq_used_elem
{
    /* Index of start of used descriptor chain. */
    uint32_t id;
    /* Total length of the descriptor chain which was used (written to) */
    uint32_t len;
};

struct virtq_used
{
#define VIRTQ_USED_F_NO_NOTIFY 1
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[QUEUE_NUM];
    uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
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
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
} __attribute__((packed));

extern int virtio_blk_init();

#endif