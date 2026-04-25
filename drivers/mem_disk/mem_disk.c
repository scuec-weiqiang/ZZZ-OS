#include <fs/blkdev.h>
#include <mm/page.h>
#include <os/check.h>
#include <os/errno.h>
#include <os/kva.h>
#include <os/mm.h>
#include <os/module.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/string.h>

#define MEM_DISK_SECTOR_SIZE 512U

struct mem_disk {
    spinlock_t lock;
    void *data;
    size_t size_bytes;
    phys_addr_t phys_base;
};

static struct mem_disk mem_disk_dev;
static struct request_queue mem_disk_queue;
static struct gendisk mem_disk_gendisk;

static int mem_disk_submit_bio(struct block_device *bdev, struct bio *bio)
{
    struct mem_disk *disk = NULL;
    sector_t sector = 0;

    CHECK(bdev != NULL, "mem_disk: invalid block device", return -EINVAL;);
    CHECK(bio != NULL, "mem_disk: invalid bio", return -EINVAL;);
    CHECK(bdev->bd_disk != NULL && bdev->bd_disk->private_data != NULL,
          "mem_disk: missing private data", return -ENODEV;);
    disk = bdev->bd_disk->private_data;

    sector = bdev_sector_offset(bdev, bio->bi_sector);

    for (int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];
        uint8_t *page_base = NULL;
        uint32_t done = 0;

        CHECK(bvec->page != NULL, "mem_disk: bio_vec page is NULL", return -EINVAL;);
        if (bvec->len == 0) {
            continue;
        }

        CHECK(bvec->offset + bvec->len <= PAGE_SIZE,
              "mem_disk: bio_vec crosses page boundary", return -EINVAL;);
        CHECK((bvec->offset % MEM_DISK_SECTOR_SIZE) == 0,
              "mem_disk: unaligned bio offset", return -EINVAL;);
        CHECK((bvec->len % MEM_DISK_SECTOR_SIZE) == 0,
              "mem_disk: unaligned bio length", return -EINVAL;);

        page_base = (uint8_t *)page_address(bvec->page) + bvec->offset;
        while (done < bvec->len) {
            uint64_t disk_off = (uint64_t)sector * MEM_DISK_SECTOR_SIZE;

            CHECK(disk_off + MEM_DISK_SECTOR_SIZE <= disk->size_bytes,
                  "mem_disk: bio exceeds disk size", return -EIO;);

            spin_lock(&disk->lock);
            if (bio->op == REQ_OP_READ) {
                memcpy(page_base + done, (uint8_t *)disk->data + disk_off, MEM_DISK_SECTOR_SIZE);
            } else if (bio->op == REQ_OP_WRITE) {
                memcpy((uint8_t *)disk->data + disk_off, page_base + done, MEM_DISK_SECTOR_SIZE);
            } else {
                spin_unlock(&disk->lock);
                return -EINVAL;
            }
            spin_unlock(&disk->lock);

            done += MEM_DISK_SECTOR_SIZE;
            sector++;
        }
    }

    return 0;
}

static int mem_disk_register(struct mem_disk *disk)
{
    static struct block_device_operations mem_disk_bdops = {
        .submit_bio = mem_disk_submit_bio,
    };
    int ret = 0;
    size_t total_sectors = 0;

    CHECK(disk != NULL, "mem_disk: invalid disk", return -EINVAL;);
    CHECK(disk->data != NULL, "mem_disk: backing store is not mapped", return -EINVAL;);
    CHECK(disk->size_bytes >= MEM_DISK_SECTOR_SIZE,
          "mem_disk: backing store is too small", return -EINVAL;);
    CHECK((disk->size_bytes % MEM_DISK_SECTOR_SIZE) == 0,
          "mem_disk: backing store size must align to sector size", return -EINVAL;);
    total_sectors = disk->size_bytes / MEM_DISK_SECTOR_SIZE;

    memset(&mem_disk_queue, 0, sizeof(mem_disk_queue));
    spin_lock_init(&mem_disk_queue.lock);
    mem_disk_queue.fops = &mem_disk_bdops;
    mem_disk_queue.logical_block_size = MEM_DISK_SECTOR_SIZE;
    mem_disk_queue.max_hw_sectors = (uint32_t)total_sectors;
    mem_disk_queue.queuedata = disk;

    memset(&mem_disk_gendisk, 0, sizeof(mem_disk_gendisk));
    strcpy(mem_disk_gendisk.disk_name, "ram_disk");
    mem_disk_gendisk.capacity = (sector_t)total_sectors;
    mem_disk_gendisk.logical_block_size = MEM_DISK_SECTOR_SIZE;
    mem_disk_gendisk.queue = &mem_disk_queue;
    mem_disk_gendisk.private_data = disk;

    ret = register_blkdev(&mem_disk_gendisk);
    CHECK(ret == 0, "mem_disk: register block device failed",
          return ret;);

    printk("mem_disk: mapped pa=%xu va=%xu size=%xu bytes\n",
           (unsigned)disk->phys_base, (unsigned)disk->data, (unsigned)disk->size_bytes);
    printk("mem_disk: registered ram_disk (%xu bytes, %xu sectors)\n",
           (unsigned)disk->size_bytes, (unsigned)total_sectors);
    return 0;
}

static int mem_disk_probe(struct platform_device *pdev)
{
    struct resource *res = NULL;

    CHECK(pdev != NULL, "mem_disk: invalid platform device", return -EINVAL;);

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    CHECK(res != NULL, "mem_disk: missing mem resource", return -ENODEV;);

    memset(&mem_disk_dev, 0, sizeof(mem_disk_dev));
    spin_lock_init(&mem_disk_dev.lock);
    mem_disk_dev.phys_base = res->start;
    mem_disk_dev.size_bytes = res->size;
    mem_disk_dev.data = (void *)platform_ioremap_resource(pdev, 0);
    CHECK(mem_disk_dev.data != NULL, "mem_disk: ioremap failed", return -ENOMEM;);

    dev_set_drvdata(&pdev->dev, &mem_disk_dev);
    return mem_disk_register(&mem_disk_dev);
}

static int mem_disk_remove(struct platform_device *pdev)
{
    struct mem_disk *disk = NULL;

    CHECK(pdev != NULL, "mem_disk: invalid platform device", return -EINVAL;);

    disk = dev_get_drvdata(&pdev->dev);
    if (disk != NULL && disk->data != NULL) {
        iounmap((virt_addr_t)disk->data, disk->size_bytes);
        disk->data = NULL;
    }
    return 0;
}

static struct of_device_id mem_disk_of_match[] = {
    { .compatible = "wq,mem-disk" },
    { /* sentinel */ }
};

static struct platform_driver mem_disk_driver = {
    .name = "mem_disk",
    .probe = mem_disk_probe,
    .remove = mem_disk_remove,
    .driver = {
        .of_match_table = mem_disk_of_match,
    },
};

module_platform_driver(mem_disk_driver);
