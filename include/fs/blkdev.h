#ifndef __FS_BLOCK_DEV_H__
#define __FS_BLOCK_DEV_H__

#include <os/types.h>
#include <os/list.h>
#include <os/spinlock.h>

typedef unsigned int sector_t;

enum bio_op {
    REQ_OP_READ = 0,
    REQ_OP_WRITE = 1,
};

struct bio_vec {
    struct page *page;
    uint32_t offset;
    uint32_t len;
};

struct bio {
    enum bio_op op;
    sector_t bi_sector;
    int bi_vcnt;
    struct bio_vec *bi_io_vec;
    struct block_device *bi_bdev;
    int bi_status;
    void (*bi_end_io)(struct bio *bio);
    void *bi_private;
};

/*
 * submit_bio() only accepts standardized block I/O:
 * - bi_sector is expressed in logical blocks of the target queue
 * - each bio_vec stays within a single page
 * - bio_vec.offset is aligned to logical_block_size
 * - bio_vec.len is a multiple of logical_block_size
 */
struct block_device_operations {
    int (*submit_bio)(struct block_device *bdev, struct bio *bio);
};

struct request_queue {
    spinlock_t lock;
    struct block_device_operations *fops;
    uint32_t logical_block_size;
    uint32_t max_hw_sectors;
    void *queuedata;
};

struct gendisk {
    char disk_name[32];
    sector_t capacity;          // 单位: sector
    uint32_t logical_block_size;
    struct request_queue *queue;
    struct block_device *part0;
    struct list_head disk_list;
    void *private_data;
};

struct block_device {
    struct gendisk *bd_disk;
    sector_t bd_start_sect;     // 分区起始 sector
    sector_t bd_nr_sectors;     // 分区大小
    int bd_openers;
    void *bd_fs_info;
};

int register_blkdev(struct gendisk *disk);
struct block_device *blkdev_get_by_path(const char *name);
void blkdev_put(struct block_device *bdev);

struct bio *bio_alloc(int nr_vecs);
void bio_put(struct bio *bio);

int submit_bio_wait(struct bio *bio);
/*
 * blkdev_read/write are byte-granularity helpers. They translate arbitrary
 * byte ranges into standardized block I/O and submit those bios downward.
 */
int blkdev_read(struct block_device *bdev, void *buf, size_t len, uint64_t pos);
int blkdev_write(struct block_device *bdev, const void *buf, size_t len, uint64_t pos);

static inline sector_t bdev_sector_offset(struct block_device *bdev, sector_t sector)
{
    return bdev->bd_start_sect + sector;
}

#endif
