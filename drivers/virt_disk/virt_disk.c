/**
 * @FilePath: /ZZZ-OS/drivers/virt_disk.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:44
 * @LastEditTime: 2025-11-24 22:20:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <asm/pgtbl.h>
#include <fs/blkdev.h>
#include <mm/physmem.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/types.h>
#include "virtio.h"

#define VIRT_DISK_QUEUE 0
#define SECTOR_SIZE 512

enum virt_disk_rw {
    VIRT_DISK_READ = 0,
    VIRT_DISK_WRITE,
};

struct virt_disk {
    spinlock_t lock;
    struct virtq disk_queue;
    char kfree[QUEUE_NUM];
    int last_used_idx;
    struct virtio_blk_req req[QUEUE_NUM];
    char status[QUEUE_NUM];

    struct disk_info {
        int capacity;
        int blk_size;
        int blk_num;
    } disk_info;
};

static struct virt_disk virt_disk;
static struct request_queue virt_disk_queue;
static struct gendisk virt_disk_gendisk;

static int alloc_disk_desc(void)
{
    for (int i = 0; i < QUEUE_NUM; i++) {
        if (virt_disk.kfree[i] == 1) {
            virt_disk.kfree[i] = 0;
            return i;
        }
    }
    return -1;
}

static void free_disk_desc(int index)
{
    if (index >= 0 && index < QUEUE_NUM) {
        virt_disk.kfree[index] = 1;
    } else {
        printk("free_disk_desc: index out of range!\n");
    }
}

static void free_disk_chain(int index)
{
    while (1) {
        uint16_t next = virt_disk.disk_queue.desc[index].next;
        uint16_t flags = virt_disk.disk_queue.desc[index].flags;

        free_disk_desc(index);
        if (flags & VIRTQ_DESC_F_NEXT) {
            index = next;
        } else {
            break;
        }
    }
}

static int virt_disk_rw(void *buffer, int sector, enum virt_disk_rw rwflag)
{
    int index[3];
    int ret = 0;

    spin_lock(&virt_disk.lock);

    for (int i = 0; i < 3; i++) {
        index[i] = alloc_disk_desc();
        if (index[i] < 0) {
            ret = -1;
            goto out_unlock;
        }
    }

    virt_disk.req[index[0]].type = (rwflag == VIRT_DISK_READ) ? VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT;
    virt_disk.req[index[0]].reserved = 0;
    virt_disk.req[index[0]].sector = sector;

    virt_disk.disk_queue.desc[index[0]].addr = KERNEL_PA((uintptr_t)&virt_disk.req[index[0]]);
    virt_disk.disk_queue.desc[index[0]].len = sizeof(struct virtio_blk_req);
    virt_disk.disk_queue.desc[index[0]].flags = VIRTQ_DESC_F_NEXT;
    virt_disk.disk_queue.desc[index[0]].next = index[1];

    virt_disk.disk_queue.desc[index[1]].addr = KERNEL_PA((uintptr_t)buffer);
    virt_disk.disk_queue.desc[index[1]].len = SECTOR_SIZE;
    virt_disk.disk_queue.desc[index[1]].flags = VIRTQ_DESC_F_NEXT |
                                                 (rwflag == VIRT_DISK_READ ? VIRTQ_DESC_F_WRITE : 0);
    virt_disk.disk_queue.desc[index[1]].next = index[2];

    virt_disk.disk_queue.desc[index[2]].addr = KERNEL_PA((uintptr_t)&virt_disk.status[index[0]]);
    virt_disk.disk_queue.desc[index[2]].len = 1;
    virt_disk.disk_queue.desc[index[2]].flags = VIRTQ_DESC_F_WRITE;
    virt_disk.disk_queue.desc[index[2]].next = 0;

    virt_disk.disk_queue.avail->ring[virt_disk.disk_queue.avail->idx % QUEUE_NUM] = index[0];
    __sync_synchronize();
    virt_disk.disk_queue.avail->idx += 1;
    __sync_synchronize();

    virtio->queue_notify = 0;

    while (virt_disk.last_used_idx == virt_disk.disk_queue.used->idx) {
    }

    virt_disk.last_used_idx = virt_disk.disk_queue.used->idx;
    ret = (virt_disk.status[index[0]] == 0) ? 0 : -1;
    free_disk_chain(index[0]);

out_unlock:
    spin_unlock(&virt_disk.lock);
    return ret;
}

static int virt_disk_submit_bio(struct block_device *bdev, struct bio *bio)
{
    sector_t sector = 0;

    CHECK(bdev != NULL, "virt_disk: invalid block device", return -1;);
    CHECK(bio != NULL, "virt_disk: invalid bio", return -1;);

    sector = bdev_sector_offset(bdev, bio->bi_sector);
    for (int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];
        uint8_t *base = NULL;
        uint32_t done = 0;

        CHECK(bvec->page != NULL, "virt_disk: bio_vec page is NULL", return -1;);
        if (bvec->len == 0) {
            continue;
        }

        CHECK(bvec->offset + bvec->len <= PAGE_SIZE,
              "virt_disk: bio_vec crosses page boundary", return -1;);
        CHECK((bvec->offset % SECTOR_SIZE) == 0, "virt_disk: unaligned bio offset", return -1;);
        CHECK((bvec->len % SECTOR_SIZE) == 0, "virt_disk: unaligned bio length", return -1;);

        base = (uint8_t *)KERNEL_VA(page_to_phys(bvec->page)) + bvec->offset;
        while (done < bvec->len) {
            int ret = virt_disk_rw(base + done, sector,
                                   bio->op == REQ_OP_READ ? VIRT_DISK_READ : VIRT_DISK_WRITE);
            CHECK(ret == 0, "virt_disk: sector I/O failed", return ret;);
            done += SECTOR_SIZE;
            sector++;
        }
    }

    return 0;
}

static int virt_disk_init(void)
{
    static struct block_device_operations virt_disk_bdops = {
        .submit_bio = virt_disk_submit_bio,
    };

    memset(&virt_disk, 0, sizeof(virt_disk));
    spin_lock_init(&virt_disk.lock);

    virtio_blk_init();

    virtio->queue_sel = VIRT_DISK_QUEUE;
    if (virtio->queue_num_max == 0 || virtio->queue_num_max < QUEUE_NUM) {
        return -1;
    }

    virt_disk.disk_queue.desc = (struct virtq_desc *)page_alloc(1);
    virt_disk.disk_queue.avail = (struct virtq_avail *)page_alloc(1);
    virt_disk.disk_queue.used = (struct virtq_used *)page_alloc(1);
    CHECK(virt_disk.disk_queue.desc != NULL &&
          virt_disk.disk_queue.avail != NULL &&
          virt_disk.disk_queue.used != NULL,
          "virt_disk: alloc virtqueue failed", return -1;);

    virtio->queue_num = QUEUE_NUM;
    {
        uint64_t desc_pa = (uint64_t)KERNEL_PA(virt_disk.disk_queue.desc);
        uint64_t avail_pa = (uint64_t)KERNEL_PA(virt_disk.disk_queue.avail);
        uint64_t used_pa = (uint64_t)KERNEL_PA(virt_disk.disk_queue.used);

        virtio->queue_desc_low = (uint32_t)(desc_pa & 0xFFFFFFFFU);
        virtio->queue_desc_high = (uint32_t)(desc_pa >> 32);
        virtio->queue_avail_low = (uint32_t)(avail_pa & 0xFFFFFFFFU);
        virtio->queue_avail_high = (uint32_t)(avail_pa >> 32);
        virtio->queue_used_low = (uint32_t)(used_pa & 0xFFFFFFFFU);
        virtio->queue_used_high = (uint32_t)(used_pa >> 32);
    }
    virtio->queue_ready = 1;
    if (!virtio->queue_ready) {
        return -1;
    }

    virtio->status |= VIRTIO_CONFIG_S_DRIVER_OK;
    __sync_synchronize();

    for (uint32_t i = 0; i < QUEUE_NUM; i++) {
        virt_disk.kfree[i] = 1;
    }

    {
        struct virtio_blk_config *info = (struct virtio_blk_config *)&virtio->config[0];

        virt_disk.disk_info.blk_size = info->blk_size;
        virt_disk.disk_info.blk_num = info->capacity;
        virt_disk.disk_info.capacity = info->capacity * info->blk_size;

        memset(&virt_disk_queue, 0, sizeof(virt_disk_queue));
        spin_lock_init(&virt_disk_queue.lock);
        virt_disk_queue.fops = &virt_disk_bdops;
        virt_disk_queue.logical_block_size = SECTOR_SIZE;
        virt_disk_queue.max_hw_sectors = 1;
        virt_disk_queue.queuedata = &virt_disk;

        memset(&virt_disk_gendisk, 0, sizeof(virt_disk_gendisk));
        strcpy(virt_disk_gendisk.disk_name, "virt_disk");
        virt_disk_gendisk.capacity = info->capacity;
        virt_disk_gendisk.logical_block_size = SECTOR_SIZE;
        virt_disk_gendisk.queue = &virt_disk_queue;
        virt_disk_gendisk.private_data = &virt_disk;
    }

    printk("virtio = %xu\n", virtio);
    return register_blkdev(&virt_disk_gendisk);
}

static int virt_disk_probe(struct platform_device *pdev)
{
    struct device_node *node = of_find_node_by_compatible("wq,virt_disk");

    (void)pdev;
    if (!node) {
        return -1;
    }

    {
        uint32_t *reg = of_read_u32_array(node, "reg", 2);
        virtio = ioremap(reg[0], reg[1]);
    }
    return virt_disk_init();
}

static int virt_disk_remove(struct platform_device *pdev)
{
    (void)pdev;
    virtio->status = 0;
    virtio = NULL;
    return 0;
}

static struct of_device_id virt_disk_of_match[] = {
    {.compatible = "wq,virt_disk"},
    {/* sentinel */}
};

static struct platform_driver virt_disk_driver = {
    .name = "virt_disk_driver",
    .probe = virt_disk_probe,
    .remove = virt_disk_remove,
    .driver = {
        .name = "virt_disk_driver",
        .of_match_table = virt_disk_of_match,
    },
};

module_platform_driver(virt_disk_driver);
