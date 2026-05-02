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

static LIST_HEAD(g_blk_disks);

static int blkdev_validate_bio(struct bio *bio)
{
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

int alloc_blkdev_region(dev_t *devt, unsigned int count) {
    static dev_t next_devt = 1; // 从 1 开始分配，0 通常保留给特殊用途

    if (!devt || count == 0)
        return -EINVAL;

    *devt = MKDEV(BLK_MAJOR_DISK, next_devt);
    next_devt += count;
    return 0;
}

int blkdev_register(char *name, dev_t devnr, struct gendisk *disk, struct file_operations *fops) {
    struct list_head *pos = NULL;
    struct blkdev *bdev = NULL;

    CHECK(disk != NULL, "blkdev: invalid disk", return -EINVAL;);
    CHECK(disk->disk_name[0] != '\0', "blkdev: empty disk name", return -EINVAL;);
    CHECK(disk->queue != NULL, "blkdev: missing request queue", return -EINVAL;);
    CHECK(disk->queue->fops != NULL && disk->queue->fops->submit_bio != NULL,
          "blkdev: missing submit_bio op", return -EINVAL;);
    CHECK(disk->logical_block_size != 0, "blkdev: invalid logical block size", return -EINVAL;);
    CHECK(disk->part0 == NULL, "blkdev: disk already registered", return -EEXIST;);

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

    int ret = devnode_register(disk->disk_name, DEV_BLOCK, devnr, fops,bdev);
    if (ret < 0) {
        kfree(bdev);
        return ret;
    }
    bdev->bd_node = devnode_lookup_by_name(disk->disk_name);
    disk->part0 = bdev;
    INIT_LIST_HEAD(&disk->disk_list);
    list_add_tail(&g_blk_disks, &disk->disk_list);
    return 0;
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

struct blkdev *blkdev_get_by_devnr(dev_t devnr)
{
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


int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos) {
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


int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos) {
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
