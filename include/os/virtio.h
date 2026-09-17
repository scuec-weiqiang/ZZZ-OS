/**
 * @FilePath     : /ZZZ-OS/include/os/virtio.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-13 22:50:42
 * @LastEditTime : 2026-09-16 19:48:21
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef _OS_VIRTIO_H
#define _OS_VIRTIO_H

#include <os/types.h>
#include <os/spinlock.h>
#include <os/device.h>

#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976 // 0x74726976
#define VIRTIO_MMIO_VERSION 0x002          // version; should be 2

#define VIRTIO_CONFIG_S_ACKNOWLEDGE  1
#define VIRTIO_CONFIG_S_DRIVER       2
#define VIRTIO_CONFIG_S_DRIVER_OK    4
#define VIRTIO_CONFIG_S_FEATURES_OK  8
#define VIRTIO_CONFIG_S_FAILED       128

/* Transport feature required by non-legacy VirtIO 1.x devices. */
#define VIRTIO_F_VERSION_1           32
#define VIRTIO_FEATURE_BIT(_bit)     (1ULL << (_bit))

struct virtio_device;
struct virtqueue;

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

struct virtio_config_ops {
    u64  (*get_features)(struct virtio_device *vdev);
    int  (*finalize_features)(struct virtio_device *vdev, u64 features);

    u32  (*get_status)(struct virtio_device *vdev);
    void (*set_status)(struct virtio_device *vdev, u32 status);
    void (*reset)(struct virtio_device *vdev);

    int  (*setup_vq)(struct virtio_device *vdev,
                     struct virtqueue *vq,
                     unsigned int index);

    void (*notify)(struct virtqueue *vq);
    
    void (*get_config)(struct virtio_device *vdev,
                   unsigned int offset,
                   void *buf,
                   unsigned int len);
};

struct virtio_device_id {
    u32 device;
    u32 vendor;
};
#define VIRTIO_DEV_ANY_ID	0xffffffff

struct virtio_device {
    struct device dev;

    struct virtio_device_id id;
    const struct virtio_config_ops *config;

    u64 features;
    struct list_head vqs;
    void *priv;
};

struct virtio_driver {
    struct device_driver driver;

    const struct virtio_device_id *id_table;
    u64 feature_mask;

    int (*probe)(struct virtio_device *vdev);
    int (*remove)(struct virtio_device *vdev);
};

struct virtio_mmio_device {
    struct virtio_device vdev;
    struct platform_device *pdev;
    volatile struct virtio_mmio_regs *base;
    int irq;
    u32 version;
};


/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as device write-only (otherwise device read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4
struct virtq_desc {
    /* Address (guest-physical). */
    u64 addr;
    /* Length. */
    u32 len;

    /* The flags as indicated above. */
    u16 flags;
    /* Next field if flags & NEXT */
    u16 next;
};

#define QUEUE_NUM 8

struct virtq_avail {
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
    u16 flags;
    u16 idx;
    u16 ring[];
    // u16 used_event; /* Only if VIRTIO_F_EVENT_IDX */
};

/* u32 is used here for ids for padding reasons. */
struct virtq_used_elem {
    /* Index of start of used descriptor chain. */
    u32 id;
    /* Total length of the descriptor chain which was used (written to) */
    u32 len;
};

#define VIRTQ_USED_F_NO_NOTIFY 1
struct virtq_used {
    u16 flags;
    u16 idx;
    struct virtq_used_elem ring[];
    // u16 avail_event; /* Only if VIRTIO_F_EVENT_IDX */
};

struct virtqueue {
    struct list_head node;
    struct virtio_device *vdev;

    // 队列索引
    unsigned int index; // 对应哪个队列
    unsigned int num; // 队列长度

    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;

    // descriptor空闲链表
    u16 free_head; // 空闲链表头, 其实是desc数组的索引
    u16 num_free;

    // 驱动本地维护的下标
    u16 avail_idx;
    u16 last_used_idx;

    // head descriptor对应哪个上层请求
    void **tokens;

    void (*callback)(struct virtqueue *vq);
    void *priv;

    spinlock_t lock;
};

// 上层传给virtqueue的buffer，virtqueue会把它们映射到desc、avail、used环形缓冲区中
struct virtio_buffer {
    void *addr;
    u32 len;
    bool device_write;
};

struct virtqueue *virtqueue_create(struct virtio_device *vdev, 
                                unsigned int index, 
                                unsigned int requested_num,
                                void (*callback)(struct virtqueue *vq));
int virtqueue_add(struct virtqueue *vq,
                  struct virtio_buffer *bufs,
                  unsigned int num_bufs,
                  void *token);
void virtqueue_kick(struct virtqueue *vq);
void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len);


#define to_virtio_device(_dev) \
    container_of(_dev, struct virtio_device, dev)

#define to_virtio_driver(_drv) \
    container_of(_drv, struct virtio_driver, driver)

#define to_virtio_mmio_device(_vdev) \
    container_of(_vdev, struct virtio_mmio_device, vdev)

static inline struct virtio_device *dev_to_virtio(struct device *_dev)
{
	return container_of(_dev, struct virtio_device, dev);
}

static inline struct virtio_driver *drv_to_virtio(struct device_driver *drv)
{
	return container_of(drv, struct virtio_driver, driver);
}

extern struct bus_type virtio_bus_type;

int virtio_device_register(struct virtio_device *vdev);
int virtio_device_unregister(struct virtio_device *vdev);

int virtio_driver_register(struct virtio_driver *vdrv);
int virtio_driver_unregister(struct virtio_driver *vdrv); 
void virtio_device_ready(struct virtio_device *vdev);

#define module_virtio_driver(__virtio_driver) \
    module_driver(__virtio_driver, virtio_driver_register, virtio_driver_unregister)
#endif
