#ifndef __FS_BLOCK_DEV_H__
#define __FS_BLOCK_DEV_H__

#include <os/types.h>
#include <os/list.h>
#include <os/spinlock.h>
#include <os/devnode.h>
#include <os/compiler.h>

struct gpt_header {
    char signature[8];      // "EFI PART"
    __le32 revision;
    __le32 header_size;
    __le32 crc32;
    __le32 reserved;

    __le64 current_lba;
    __le64 backup_lba;

    __le64 first_usable_lba;
    __le64 last_usable_lba;

    u8 disk_guid[16];

    __le64 partition_entries_lba;

    __le32 num_partition_entries;
    __le32 sizeof_partition_entry;
    __le32 partition_entries_crc32;
} __packed;

struct gpt_entry {
    u8 type_guid[16];
    u8 unique_guid[16];

    __le64 first_lba;
    __le64 last_lba;

    __le64 attributes;

    __le16 name[36];
} __packed;


#define BLK_MAJOR_DISK 8
typedef unsigned int sector_t;

enum bio_op {
    REQ_OP_READ = 0,
    REQ_OP_WRITE = 1,
};

struct bio_vec {
    struct page *page;
    u32 offset;
    u32 len;
};

struct bio {
    enum bio_op op;
    sector_t bi_sector;
    int bi_vcnt;
    struct bio_vec *bi_io_vec;
    struct blkdev *bi_bdev;
    int bi_status;
    void (*bi_end_io)(struct bio *bio);
    void *bi_private;
};

struct block_device_operations {
    int (*submit_bio)(struct blkdev *bdev, struct bio *bio);
};

struct request_queue {
    spinlock_t lock;
    struct block_device_operations *fops;
    u32 logical_block_size;
    u32 max_hw_sectors;
    void *queuedata;
};

struct gendisk {
    char disk_name[32];
    sector_t capacity;          // 单位: sector
    u32 logical_block_size;
    struct request_queue *queue;
    struct blkdev *part0;
    struct list_head disk_list;
    void *private_data;
};

struct blkdev {
    struct gendisk *bd_disk;
    
    sector_t bd_start_sect;     // 分区起始 sector
    sector_t bd_nr_sectors;     // 分区大小
    int bd_partno;              // 0 = 整盘，1/2/3 = 分区
    struct blkdev *bd_contains; // 分区指向整盘，整盘指向自己

    int bd_openers;
    void *bd_fs_info;
    dev_t bd_devnr;
    struct devnode *bd_node;
};
int alloc_blkdev_region(dev_t *devt, unsigned int count);
int blkdev_register(char *name, dev_t devnr, struct gendisk *disk, struct file_operations *fops);
struct blkdev *blkdev_get_by_path(const char *name);
struct blkdev *blkdev_get_by_devnr(dev_t devnr);
void blkdev_put(struct blkdev *bdev);

struct bio *bio_alloc(int nr_vecs);
void bio_put(struct bio *bio);

int submit_bio_wait(struct bio *bio);

int blkdev_read(struct blkdev *bdev, void *buf, size_t len, u64 pos);
int blkdev_write(struct blkdev *bdev, const void *buf, size_t len, u64 pos);

static inline sector_t bdev_sector_offset(struct blkdev *bdev, sector_t sector)
{
    return bdev->bd_start_sect + sector;
}

#define GPT_HEADER_LBA 1
#define SECTOR_SIZE 512

int blkdev_scan_partitions(struct blkdev *whole);
int blkdev_register_partition(struct blkdev *whole,
                              int partno,
                              sector_t start,
                              sector_t nr_sectors);

#endif
