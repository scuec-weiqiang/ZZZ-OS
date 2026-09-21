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

static int blkdev_validate_bio(struct bio *bio) 
{
    u32 block_size = 0;
    u64 vec_bytes = 0;

    ASSERT(bio != NULL, "blkdev: invalid bio");
    if (!bio->bi_bdev) {
        printk("%s\n", "blkdev: bio missing block device");
        return -ENODEV;
    }
    if (!bio->bi_bdev->bd_disk) {
        printk("%s\n", "blkdev: bio missing disk");
        return -ENODEV;
    }
    if (!bio->bi_bdev->bd_disk->queue) {
        printk("%s\n", "blkdev: bio missing queue");
        return -ENODEV;
    }
    if (!(bio->bi_bdev->bd_disk->queue->fops != NULL &&
          bio->bi_bdev->bd_disk->queue->fops->submit_bio != NULL)) {
        printk("%s\n", "blkdev: submit_bio op missing");
        return -ENODEV;
    }

    block_size = bio->bi_bdev->bd_disk->queue->logical_block_size;
    if (!block_size) {
        printk("%s\n", "blkdev: invalid logical block size");
        return -EINVAL;
    }
    ASSERT(bio->bi_vcnt != 0 && bio->bi_vcnt <= bio->bi_max_vecs, "blkdev: invalid bio vec count");
    if (!(bio->bi_size != 0 && mod_u32(bio->bi_size, block_size) == 0)) {
        printk("%s\n", "blkdev: unaligned bio size");
        return -EINVAL;
    }
    if (!(mod_u64(bio->bi_sector * SECTOR_SIZE, block_size) == 0)) {
        printk("%s\n", "blkdev: unaligned bio sector");
        return -EINVAL;
    }

    for (int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];

        ASSERT(bvec->page != NULL, "blkdev: bio_vec page is NULL");
        if (bvec->len == 0) {
            continue;
        }

        ASSERT(bvec->offset + bvec->len <= PAGE_SIZE, "blkdev: bio_vec crosses page boundary");
        vec_bytes += bvec->len;
    }

    ASSERT(vec_bytes == bio->bi_size, "blkdev: bio size does not match vectors");

    return 0;
}

static dev_t next_devt = 1; // 从 1 开始分配，0 通常保留给特殊用途

int alloc_blkdev_region(dev_t *devt, unsigned int count) 
{
    if (!devt || count == 0)
        return -EINVAL;

    *devt = MKDEV(BLK_MAJOR_DISK, next_devt);
    next_devt += count;
    return 0;
}

static void update_region() 
{
    next_devt++;
}

struct blkdev *blkdev_get_by_path(const char *path) 
{
    struct device *dev;
    struct blkdev *bdev;

    if (!path)
        return ERR_PTR(-EINVAL);

    if (strncmp(path, "/dev/", 5) == 0)
        path += 5;

    dev = device_find_by_name(path);
    if (!dev)
        return ERR_PTR(-ENODEV);

    if (!S_ISBLK(dev->mode))
        return ERR_PTR(-ENOTBLK);

    bdev = dev_get_drvdata(dev);
    if (!bdev)
        return ERR_PTR(-ENODEV);

    bdev->bd_openers++;
    return bdev;
}

struct blkdev *blkdev_get_by_devnr(dev_t devnr) 
{
    struct device *dev;
    struct blkdev *bdev;

    dev = device_find_by_devt(devnr);
    if (!dev)
        return NULL;

    if (!S_ISBLK(dev->mode))
        return NULL;

    bdev = dev_get_drvdata(dev);
    if (!bdev)
        return NULL;

    return bdev;
}

void blkdev_put(struct blkdev *bdev) 
{
    if (bdev == NULL) {
        return;
    }

    if (bdev->bd_openers > 0) {
        bdev->bd_openers--;
    }
}

struct bio *bio_alloc(u32 nr_vecs) 
{
    struct bio *bio = NULL;
    if (nr_vecs == 0 || nr_vecs > UINT16_MAX) {
        return ERR_PTR(-EINVAL);
    }

    bio = kzalloc(sizeof(*bio));
    if (!bio) {
        return ERR_PTR(-ENOMEM);
    }

    if (nr_vecs > 0) {
        bio->bi_io_vec = kzalloc(sizeof(*bio->bi_io_vec) * nr_vecs);
        if (bio->bi_io_vec == NULL) {
            kfree(bio);
            return ERR_PTR(-ENOMEM);
        }
    }

    bio->bi_max_vecs = nr_vecs;
    bio->bi_vcnt = 0;
    bio->bi_bdev = NULL;
    bio->bi_sector = 0;
    bio->bi_size = 0;
    bio->bi_status = 0;
    return bio;
}

void bio_put(struct bio *bio) 
{
    if (bio == NULL) {
        return;
    }

    if (bio->bi_io_vec != NULL) {
        kfree(bio->bi_io_vec);
    }
    kfree(bio);
}

int bio_add_page(struct bio *bio, struct page *page, u32 len, u32 offset)
{
    struct bio_vec *bvec;

    if (!bio || !page || len == 0)
        return -EINVAL;

    if (offset >= PAGE_SIZE || len > PAGE_SIZE - offset)
        return -EINVAL;

    if (bio->bi_vcnt >= bio->bi_max_vecs)
        return -ENOSPC;

    if (len > UINT32_MAX - bio->bi_size)
        return -EOVERFLOW;

    bvec = &bio->bi_io_vec[bio->bi_vcnt++];
    bvec->page = page;
    bvec->offset = offset;
    bvec->len = len;
    bio->bi_size += len;

    return 0;
}

static int bio_map_kern_buf(struct bio *bio, void *buf, size_t len)
{
    u8 *cursor = buf;

    while (len != 0) {
        u32 offset = (uintptr_t)cursor & (PAGE_SIZE - 1);
        size_t chunk = PAGE_SIZE - offset;
        struct page *page;
        int ret;

        if (chunk > len)
            chunk = len;

        page = address_page(cursor);
        if (!page)
            return -EFAULT;

        ret = bio_add_page(bio, page, (u32)chunk, offset);
        if (ret)
            return ret;

        cursor += chunk;
        len -= chunk;
    }

    return 0;
}

int submit_bio_wait(struct bio *bio) 
{
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

static int blkdev_write_partial_block(struct blkdev *bdev, const void *src, size_t len, u64 pos)
{
    int ret = 0;
    struct request_queue *queue = bdev->bd_disk->queue;
    u32 block_size = queue->logical_block_size;
    if (block_size == 0 || block_size > PAGE_SIZE)
        return -EINVAL;

    size_t offset = pos % block_size;
    u64 block_pos = pos - offset;
    if (len > block_size - offset)
        return -EINVAL;

    void *bounce = page_alloc(1);
    if (!bounce)
        return -ENOMEM;

    struct page *page = address_page(bounce);
    if (!page) {
        ret = -EFAULT;
        goto out_page;
    }

    struct bio *bio = bio_alloc(1);
    if (IS_ERR(bio)) {
        ret = PTR_ERR(bio);
        goto out_page;
    }

    bio->bi_bdev = bdev;
    bio->bi_sector = block_pos / SECTOR_SIZE; //bi_sector 的单位始终定义为512字节扇区。
    bio->op = REQ_OP_READ;
    ret = bio_add_page(bio, page, block_size, 0);
    if (ret) {
        goto out_bio;
    }

    // 读取、修改、写回需要加锁，防止其他线程同时访问同一个块
    mutex_lock(&queue->rmw_lock);
    // 先读
    ret = submit_bio_wait(bio);
    if (ret) {
        mutex_unlock(&queue->rmw_lock);
        goto out_bio;
    }
    // 修改
    memcpy((u8 *)bounce + offset, src, len);
    // 写回
    bio->op = REQ_OP_WRITE;
    ret = submit_bio_wait(bio);
    mutex_unlock(&queue->rmw_lock);

out_bio:
    bio_put(bio);
out_page:
    page_free(bounce);
    return ret;
}

static int blkdev_write_aligned(struct blkdev *bdev, const void *buf, size_t len, u64 pos, size_t *written)
{
    struct bio *bio = NULL;
    size_t first_offset;
    size_t nr_vecs;
    int ret = 0;

    if (!bdev || !buf || len == 0)
        return -EINVAL;

    if (len > UINT32_MAX)
        return -EOVERFLOW;

    first_offset = (uintptr_t)buf & (PAGE_SIZE - 1);
    nr_vecs = (first_offset + len + PAGE_SIZE - 1) / PAGE_SIZE;

    bio = bio_alloc(nr_vecs);
    if (IS_ERR(bio)) {
        return PTR_ERR(bio);
    }

    bio->bi_bdev = bdev;
    bio->bi_sector = pos / SECTOR_SIZE; //bi_sector 的单位始终定义为512字节扇区。
    ret = bio_map_kern_buf(bio, (void *)buf, len);
    if (ret) {
        goto out_bio;
    }

    bio->op = REQ_OP_WRITE;
    ret = submit_bio_wait(bio);
    if (written) {
        *written = bio->bi_size;
    }
out_bio:
    bio_put(bio);
    return ret;
}

static int blkdev_read_partial_block(struct blkdev *bdev, void *dst, size_t len, u64 pos)
{
    struct request_queue *queue;
    struct bio *bio = NULL;
    struct page *page;
    void *bounce = NULL;
    u32 block_size;
    u32 offset;
    u64 block_pos;
    int ret;

    if (!bdev || !bdev->bd_disk || !bdev->bd_disk->queue ||
        !dst || len == 0)
        return -EINVAL;

    queue = bdev->bd_disk->queue;
    block_size = queue->logical_block_size;

    if (block_size == 0 || block_size > PAGE_SIZE)
        return -EINVAL;

    offset = pos % block_size;
    block_pos = pos - offset;

    if (len > block_size - offset)
        return -EINVAL;

    bounce = page_alloc(1);
    if (!bounce)
        return -ENOMEM;

    page = address_page(bounce);
    if (!page) {
        ret = -EFAULT;
        goto out_page;
    }

    bio = bio_alloc(1);
    if (IS_ERR(bio)) {
        ret = PTR_ERR(bio);
        bio = NULL;
        goto out_page;
    }

    bio->bi_bdev = bdev;
    bio->bi_sector = block_pos / SECTOR_SIZE;
    bio->op = REQ_OP_READ;

    ret = bio_add_page(bio, page, block_size, 0);
    if (ret)
        goto out_bio;

    ret = submit_bio_wait(bio);
    if (ret)
        goto out_bio;

    memcpy(dst, (u8 *)bounce + offset, len);

out_bio:
    bio_put(bio);
out_page:
    page_free(bounce);
    return ret;
}

static int blkdev_read_aligned(struct blkdev *bdev, void *buf, size_t len, u64 pos, size_t *read_bytes)
{
    struct bio *bio = NULL;
    size_t first_offset;
    size_t nr_vecs;
    int ret = 0;

    if (!bdev || !buf || len == 0)
        return -EINVAL;

    if (len > UINT32_MAX)
        return -EOVERFLOW;

    first_offset = (uintptr_t)buf & (PAGE_SIZE - 1);
    nr_vecs = (first_offset + len + PAGE_SIZE - 1) / PAGE_SIZE;

    bio = bio_alloc(nr_vecs);
    if (IS_ERR(bio)) {
        return PTR_ERR(bio);
    }

    bio->bi_bdev = bdev;
    bio->bi_sector = pos / SECTOR_SIZE; //bi_sector 的单位始终定义为512字节扇区。
    ret = bio_map_kern_buf(bio, buf, len);
    if (ret) {
        goto out_bio;
    }

    bio->op = REQ_OP_READ;
    ret = submit_bio_wait(bio);
    if (!ret && read_bytes)
        *read_bytes = bio->bi_size;

out_bio:
    bio_put(bio);
    return ret;
}


int __blkdev_read_raw(struct blkdev *bdev, void *buf, size_t len, u64 pos) 
{
    u8 *dst = buf;
    struct request_queue *queue;
    u32 block_size;
    size_t done = 0;
    int ret;

    if (!bdev || !buf)
        return -EINVAL;

    if (len == 0)
        return 0;

    queue = bdev->bd_disk->queue;
    block_size = queue->logical_block_size;

    // 非对齐头部。
    if (pos % block_size) {
        size_t head_len;

        head_len = block_size - pos % block_size;
        if (head_len > len)
            head_len = len;

        ret = blkdev_read_partial_block(
            bdev, dst, head_len, pos);
        if (ret)
            return ret;

        dst += head_len;
        pos += head_len;
        done += head_len;
    }

    // 如果整个请求都落在第一个非对齐逻辑块里，
    if (done == len)
        return 0;

    //  中间完整逻辑块。
    if (len - done >= block_size) {
        size_t aligned_len;
        size_t read_bytes;

        aligned_len = (len - done) -
                      ((len - done) % block_size);

        ret = blkdev_read_aligned(bdev, dst,
                                   aligned_len, pos,
                                   &read_bytes);
        if (ret)
            return ret;

        dst += read_bytes;
        pos += read_bytes;
        done += read_bytes;
    }

    // 非对齐尾部,此时 pos 已经位于逻辑块起点，但剩余长度不足一个块。
    if (done < len) {
        ret = blkdev_read_partial_block(
            bdev, dst, len - done, pos);
        if (ret)
            return ret;
    }

    return 0;
}

int __blkdev_write_raw(struct blkdev *bdev, const void *buf, size_t len, u64 pos)
{
    const u8 *src = buf;
    struct request_queue *queue;
    u32 block_size;
    size_t done = 0;
    int ret;

    if (!bdev || !buf)
        return -EINVAL;

    if (len == 0)
        return 0;

    queue = bdev->bd_disk->queue;
    block_size = queue->logical_block_size;

    // 非对齐头部。
    if (pos % block_size) {
        size_t head_len;

        head_len = block_size - pos % block_size;
        if (head_len > len)
            head_len = len;

        ret = blkdev_write_partial_block(
            bdev, src, head_len, pos);
        if (ret)
            return ret;

        src += head_len;
        pos += head_len;
        done += head_len;
    }

    // 如果整个请求都落在第一个非对齐逻辑块里，
    if (done == len)
        return 0;

    //  中间完整逻辑块。
    if (len - done >= block_size) {
        size_t aligned_len;
        size_t written;

        aligned_len = (len - done) -
                      ((len - done) % block_size);

        ret = blkdev_write_aligned(bdev, src,
                                   aligned_len, pos,
                                   &written);
        if (ret)
            return ret;

        src += written;
        pos += written;
        done += written;
    }

    // 非对齐尾部,此时 pos 已经位于逻辑块起点，但剩余长度不足一个块。
    if (done < len) {
        ret = blkdev_write_partial_block(
            bdev, src, len - done, pos);
        if (ret)
            return ret;
    }

    return 0;
}

int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos) 
{
    u64 size;
    u64 real_pos;

    size = (u64)bdev->bd_nr_sectors * SECTOR_SIZE;

    if (pos + len > size) {
        return -EINVAL; 
    }
        

    real_pos = (u64)bdev->bd_start_sect * SECTOR_SIZE + pos;

    return __blkdev_read_raw(bdev->bd_contains, buf, len, real_pos);
}

int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos) 
{
    u64 size;
    u64 real_pos;

    size = (u64)bdev->bd_nr_sectors * SECTOR_SIZE;

    if (pos + len > size)
        return -EINVAL;

    // 之前的pos是相对于分区的偏移量，需要转换为相对于整个磁盘的偏移量
    real_pos = (u64)bdev->bd_start_sect * SECTOR_SIZE + pos;

    return __blkdev_write_raw(bdev->bd_contains, buf, len, real_pos);
}


static int guid_is_zero(const u8 guid[16]) 
{
    int i;

    for (i = 0; i < 16; i++) {
        if (guid[i])
            return 0;
    }

    return 1;
}

int blkdev_register_partition(struct blkdev *whole, 
                              int partno, 
                              sector_t start, 
                              sector_t nr_sectors) 
{
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
    part->bd_fops = whole->bd_fops;

    snprintf(name, sizeof(name), "%s%d", whole->bd_disk->disk_name, partno);

    part->bd_devnr = MKDEV(MAJOR(whole->bd_devnr), MINOR(whole->bd_devnr) + partno);
    update_region();

    part->bd_device = device_create(whole->bd_device->class,
                                    whole->bd_device, part->bd_devnr,
                                    S_IFBLK | 0600, part, name);
    if (IS_ERR(part->bd_device)) {
        int ret = PTR_ERR(part->bd_device);
        kfree(part);
        return ret;
    }

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
    if (!bdev) {
        printk("%s\n", "blkdev: alloc block device failed");
        return -ENOMEM;
    }

    bdev->bd_disk = disk;
    bdev->bd_start_sect = 0;
    bdev->bd_nr_sectors = disk->capacity;
    bdev->bd_openers = 0;
    bdev->bd_fs_info = NULL;
    bdev->bd_devnr = devnr; 
    bdev->bd_contains = bdev;
    bdev->bd_fops = fops;

    static struct class block_class = { .name = "block" };
    static bool block_class_ready;
    int ret;

    if (!block_class_ready) {
        ret = class_register(&block_class);
        if (ret && ret != -EEXIST) {
            kfree(bdev);
            return ret;
        }
        block_class_ready = true;
    }

    bdev->bd_device = device_create(&block_class, NULL, devnr,
                                    S_IFBLK | 0600, bdev, name);
    if (IS_ERR(bdev->bd_device)) {
        ret = PTR_ERR(bdev->bd_device);
        kfree(bdev);
        return ret;
    }
    disk->part0 = bdev;
    INIT_LIST_HEAD(&disk->disk_list);
    list_add_tail(&g_blk_disks, &disk->disk_list);
    blkdev_scan_partitions(bdev);

    return 0;
}
