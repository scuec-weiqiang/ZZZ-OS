#include <fs/blkdev.h>
#include <os/devnode.h>
#include <os/check.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/kva.h>
#include <mm/buddy.h>
#include <mm/page.h>
#include <os/utils.h>
#include <os/printk.h>

static LIST_HEAD(g_blk_disks);

static int blkdev_validate_bio(struct bio *bio) {
    u32 block_size = 0;

    CHECK(bio != NULL, "blkdev: invalid bio", return -EINVAL;);
    CHECK(bio->bi_bdev != NULL, "blkdev: bio missing block device", return -ENODEV;);
    CHECK(bio->bi_bdev->bd_disk != NULL, "blkdev: bio missing disk", return -ENODEV;);
    CHECK(bio->bi_bdev->bd_disk->queue != NULL, "blkdev: bio missing queue", return -ENODEV;);
    CHECK(bio->bi_bdev->bd_disk->queue->fops != NULL &&
          bio->bi_bdev->bd_disk->queue->fops->submit_bio != NULL,
          "blkdev: submit_bio op missing", return -ENODEV;);

    block_size = bio->bi_bdev->bd_disk->queue->logical_block_size;
    CHECK(block_size != 0, "blkdev: invalid logical block size", return -EINVAL;);
    CHECK(bio->bi_vcnt >= 0, "blkdev: invalid bio vec count", return -EINVAL;);

    for (int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];

        CHECK(bvec->page != NULL, "blkdev: bio_vec page is NULL", return -EINVAL;);
        if (bvec->len == 0) {
            continue;
        }

        CHECK(bvec->offset + bvec->len <= PAGE_SIZE,
              "blkdev: bio_vec crosses page boundary", return -EINVAL;);
        CHECK(mod_u32(bvec->offset, block_size) == 0,
              "blkdev: unaligned bio offset", return -EINVAL;);
        CHECK(mod_u32(bvec->len, block_size) == 0,
              "blkdev: unaligned bio length", return -EINVAL;);
    }

    return 0;
}

static dev_t next_devt = 1; // 从 1 开始分配，0 通常保留给特殊用途

int alloc_blkdev_region(dev_t *devt, unsigned int count) {
    if (!devt || count == 0)
        return -EINVAL;

    *devt = MKDEV(BLK_MAJOR_DISK, next_devt);
    next_devt += count;
    return 0;
}

static void update_region() {
    next_devt++;
}

struct blkdev *blkdev_get_by_path(const char *path) {
    struct devnode *node;
    struct blkdev *bdev;

    if (!path)
        return ERR_PTR(-EINVAL);

    if (strncmp(path, "/dev/", 5) == 0)
        path += 5;

    node = devnode_lookup_by_name(path);
    if (!node)
        return ERR_PTR(-ENODEV);

    if (node->type != DEV_BLOCK)
        return ERR_PTR(-ENOTBLK);

    bdev = node->private;
    if (!bdev)
        return ERR_PTR(-ENODEV);

    bdev->bd_openers++;
    return bdev;
}

struct blkdev *blkdev_get_by_devnr(dev_t devnr) {
    struct devnode *node;
    struct blkdev *bdev;

    node = devnode_lookup_by_devnr(devnr);
    if (!node)
        return NULL;

    if (node->type != DEV_BLOCK)
        return NULL;

    bdev = node->private;
    if (!bdev)
        return NULL;

    return bdev;
}

void blkdev_put(struct blkdev *bdev) {
    if (bdev == NULL) {
        return;
    }

    if (bdev->bd_openers > 0) {
        bdev->bd_openers--;
    }
}

struct bio *bio_alloc(int nr_vecs) {
    struct bio *bio = NULL;

    RETURN_VAL_IF(nr_vecs < 0, ERR_PTR(-EINVAL));

    bio = kzalloc(sizeof(*bio));
    RETURN_VAL_IF(bio == NULL, ERR_PTR(-ENOMEM));

    if (nr_vecs > 0) {
        bio->bi_io_vec = kzalloc(sizeof(*bio->bi_io_vec) * nr_vecs);
        if (bio->bi_io_vec == NULL) {
            kfree(bio);
            return ERR_PTR(-ENOMEM);
        }
    }

    bio->bi_vcnt = nr_vecs;
    bio->bi_status = 0;
    return bio;
}

void bio_put(struct bio *bio) {
    if (bio == NULL) {
        return;
    }

    if (bio->bi_io_vec != NULL) {
        kfree(bio->bi_io_vec);
    }
    kfree(bio);
}

int submit_bio_wait(struct bio *bio) {
    int ret = 0;

    ret = blkdev_validate_bio(bio);
    if (ret != 0) {
        return ret;
    }

    ret = bio->bi_bdev->bd_disk->queue->fops->submit_bio(bio->bi_bdev, bio);
    bio->bi_status = ret;
    if (bio->bi_end_io != NULL) {
        bio->bi_end_io(bio);
    }
    return ret;
}

int __blkdev_read_raw(struct blkdev *bdev, void *buf, size_t len, u64 pos) {
    struct bio *bio = NULL;
    void *base = NULL;
    struct page *temp = NULL;
    u8 *dst = buf;
    u32 sector_size = 0;
    u32 sectors_per_page = 0;
    u32 sector = 0;
    size_t done = 0;
    int ret = 0;

    if (!bdev || !buf) {
        return -EINVAL;
    }
    if (len == 0) {
        return 0;
    }

    sector_size = bdev->bd_disk->logical_block_size;
    sectors_per_page = div_u32(PAGE_SIZE, sector_size);
    sector = div_u64(pos, sector_size);

    bio = bio_alloc(1);

    if (IS_ERR(bio)) {
        return PTR_ERR(bio);
    }

    base = page_alloc(1);
    if (!base) {
        ret = -ENOMEM;
        goto out_bio;
    }
    temp = address_page(base);

    bio->bi_bdev = bdev;
    bio->bi_io_vec[0].page = temp;
    bio->bi_io_vec[0].len = sector_size;
    bio->op = REQ_OP_READ;

    while (done < len) {
        size_t offset_in_sector = mod_u64(pos, sector_size);
        size_t copy_len = len - done;
        u32 sector_offset_in_page = mod_u32(sector, sectors_per_page) * sector_size;
        u8 *src = (u8 *)base + sector_offset_in_page + offset_in_sector;

        if (copy_len > sector_size - offset_in_sector) {
            copy_len = sector_size - offset_in_sector;
        }

        bio->bi_sector = sector;
        bio->bi_io_vec[0].offset = sector_offset_in_page;

        ret = submit_bio_wait(bio);
        if (ret != 0) {
            ret = -EIO;
            goto out_page;
        }

        memcpy(dst + done, src, copy_len);
        done += copy_len;
        pos += copy_len;
        sector++;
    }

    ret = 0;
out_page:
    free_pages_kva(base);
out_bio:
    bio_put(bio);
    return ret;
}

int __blkdev_write_raw(struct blkdev *bdev, const void *buf, size_t len, u64 pos) {
    struct bio *bio = NULL;
    void *base = NULL;
    struct page *temp = NULL;
    const u8 *src = buf;
    u32 sector_size = 0;
    u32 sectors_per_page = 0;
    u32 sector = 0;
    size_t done = 0;
    int ret = 0;

    if (!bdev || !buf) {
        return -EINVAL;
    }
    if (len == 0) {
        return 0;
    }

    sector_size = bdev->bd_disk->logical_block_size;
    sectors_per_page = div_u32(PAGE_SIZE, sector_size);
    sector = div_u64(pos, sector_size);

    bio = bio_alloc(1);
    if (IS_ERR(bio)) {
        return PTR_ERR(bio);
    }

    base = page_alloc(1);
    if (!base) {
        ret = -ENOMEM;
        goto out_bio;
    }
    temp = phys_to_page(KERNEL_PA(base));

    bio->bi_bdev = bdev;
    bio->bi_io_vec[0].page = temp;
    bio->bi_io_vec[0].len = sector_size;

    while (done < len) {
        size_t offset_in_sector = mod_u64(pos, sector_size);
        size_t copy_len = len - done;
        u32 sector_offset_in_page = mod_u32(sector, sectors_per_page) * sector_size;
        u8 *dst = (u8 *)base + sector_offset_in_page;

        if (copy_len > sector_size - offset_in_sector) {
            copy_len = sector_size - offset_in_sector;
        }

        bio->bi_sector = sector;
        bio->bi_io_vec[0].offset = sector_offset_in_page;
        bio->op = REQ_OP_WRITE;

        if (offset_in_sector != 0 || copy_len != sector_size) {
            bio->op = REQ_OP_READ;
            ret = submit_bio_wait(bio);
            if (ret != 0) {
                ret = -EIO;
                goto out_page;
            }
            bio->op = REQ_OP_WRITE;
        }

        memcpy(dst + offset_in_sector, src + done, copy_len);

        ret = submit_bio_wait(bio);
        if (ret != 0) {
            ret = -EIO;
            goto out_page;
        }

        done += copy_len;
        pos += copy_len;
        sector++;
    }

    ret = 0;
out_page:
    free_pages_kva(base);
out_bio:
    bio_put(bio);
    return ret;
}

int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos) {
    u64 size;
    u64 real_pos;

    size = (u64)bdev->bd_nr_sectors * SECTOR_SIZE;

    if (pos + len > size) {
        return -EINVAL; 
    }
        

    real_pos = (u64)bdev->bd_start_sect * SECTOR_SIZE + pos;

    return __blkdev_read_raw(bdev->bd_contains, buf, len, real_pos);
}

int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos) {
    u64 size;
    u64 real_pos;

    size = (u64)bdev->bd_nr_sectors * SECTOR_SIZE;

    if (pos + len > size)
        return -EINVAL;

    real_pos = (u64)bdev->bd_start_sect * SECTOR_SIZE + pos;

    return __blkdev_write_raw(bdev->bd_contains, buf, len, real_pos);
}

static int guid_is_zero(const u8 guid[16]) {
    int i;

    for (i = 0; i < 16; i++) {
        if (guid[i])
            return 0;
    }

    return 1;
}

int blkdev_register_partition(struct blkdev *whole, int partno, sector_t start, sector_t nr_sectors) {
    struct blkdev *part;
    char name[64];

    part = kzalloc(sizeof(*part));
    if (!part)
        return -ENOMEM;

    part->bd_disk = whole->bd_disk;
    part->bd_start_sect = start;
    part->bd_nr_sectors = nr_sectors;
    part->bd_partno = partno;
    part->bd_contains = whole;

    snprintk(name, sizeof(name), "%s%d", whole->bd_disk->disk_name, partno);

    part->bd_devnr = MKDEV(MAJOR(whole->bd_devnr), MINOR(whole->bd_devnr) + partno);
    update_region();

    int ret = devnode_register(name, DEV_BLOCK, part->bd_devnr, whole->bd_node->fops, part);
    if (ret < 0) {
        kfree(part);
        return ret;
    }
    part->bd_node = devnode_lookup_by_name(name);

    return 0;
}

static int gpt_scan(struct blkdev *whole) {
    struct gpt_header hdr;
    u32 nr_entries;
    u32 entry_size;
    u64 entries_pos;
    u32 i;

    if (blkdev_read(whole, &hdr, sizeof(hdr), GPT_HEADER_LBA * SECTOR_SIZE) < 0)
        return -EIO;

    if (memcmp(hdr.signature, "EFI PART", 8) != 0)
        return -EINVAL;
    dprintk("GPT signature found on %s\n", whole->bd_disk->disk_name);
    nr_entries = hdr.num_partition_entries;
    entry_size = hdr.sizeof_partition_entry;
    entries_pos = hdr.partition_entries_lba * SECTOR_SIZE;

    if (entry_size < sizeof(struct gpt_entry))
        return -EINVAL;

    for (i = 0; i < nr_entries; i++) {
        struct gpt_entry ent;
        u64 pos;
        sector_t first;
        sector_t last;
        sector_t nr;

        pos = entries_pos + (u64)i * entry_size;

        if (blkdev_read(whole, &ent, sizeof(ent), pos) < 0)
            return -EIO;

        if (guid_is_zero(ent.type_guid))
            continue;

        first = ent.first_lba;
        last = ent.last_lba;

        if (first == 0 || last < first)
            continue;

        nr = last - first + 1;

        blkdev_register_partition(whole, i + 1, first, nr);
    }

    return 0;
}

int blkdev_scan_partitions(struct blkdev *whole) {
    if (!whole || whole->bd_partno != 0)
        return -EINVAL;
    
    if (gpt_scan(whole) == 0)
        return 0;

    return -ENOENT;
}

int blkdev_register(char *name, dev_t devnr, struct gendisk *disk, struct file_operations *fops) {
    struct list_head *pos = NULL;
    struct blkdev *bdev = NULL;

    list_for_each(pos, &g_blk_disks) {
        struct gendisk *iter = list_entry(pos, struct gendisk, disk_list);
        if (strcmp(iter->disk_name, disk->disk_name) == 0) {
            return -EEXIST;
        }
    }
    bdev = kzalloc(sizeof(*bdev));
    CHECK(bdev != NULL, "blkdev: alloc block device failed", return -ENOMEM;);

    bdev->bd_disk = disk;
    bdev->bd_start_sect = 0;
    bdev->bd_nr_sectors = disk->capacity;
    bdev->bd_openers = 0;
    bdev->bd_fs_info = NULL;
    bdev->bd_devnr = devnr; 
    bdev->bd_contains = bdev;

    int ret = devnode_register(name, DEV_BLOCK, devnr, fops,bdev);
    if (ret < 0) {
        kfree(bdev);
        return ret;
    }
    bdev->bd_node = devnode_lookup_by_name(name);
    disk->part0 = bdev;
    INIT_LIST_HEAD(&disk->disk_list);
    list_add_tail(&g_blk_disks, &disk->disk_list);
    blkdev_scan_partitions(bdev);

    return 0;
}
