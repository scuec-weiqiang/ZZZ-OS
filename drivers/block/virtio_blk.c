#include <os/virtio.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/mutex.h>
#include <os/string.h>
#include <fs/blkdev.h>
#include <mm/page.h>

#define VIRTIO_ID_BLOCK 2

#define VIRTIO_BLK_T_IN 0  // read the disk
#define VIRTIO_BLK_T_OUT 1 // write the disk

#define VIRTIO_BLK_S_OK     0
#define VIRTIO_BLK_S_IOERR  1
#define VIRTIO_BLK_S_UNSUPP 2

struct virtio_blk {
    struct virtio_device *vdev;
    struct virtqueue *vq;
    struct mutex lock;
    u64 capacity;
    struct request_queue queue;
    struct gendisk disk;
};

struct virtio_blk_config {
    u64 capacity;
} __attribute__((packed));

struct virtio_blk_req{
    struct virtio_blk_req_header
    {
        u32 type;
        u32 reserved;
        u64 sector;
    } __attribute__((packed)) header;
    u8 status;
} __attribute__((packed));

static const struct virtio_device_id virtio_blk_ids[] = {
    {
        .device = VIRTIO_ID_BLOCK,
        .vendor = VIRTIO_DEV_ANY_ID,
    },
    { 0 }
};

static int virtio_blk_rw(struct virtio_blk *blk, void *buffer, u32 len,
                  u64 sector, enum req_op op)
{
    struct virtio_blk_req req;
    struct virtio_buffer bufs[3];
    unsigned int used_len;
    void *token;
    int ret;

    if (!blk || !blk->vq || !buffer)
        return -EINVAL;
    if (op != REQ_OP_READ && op != REQ_OP_WRITE)
        return -EINVAL;

    req.header.type = op == REQ_OP_READ ?
                      VIRTIO_BLK_T_IN : VIRTIO_BLK_T_OUT;
    req.header.reserved = 0;
    req.header.sector = sector;
    req.status = 0xff;

    bufs[0].addr = &req.header;
    bufs[0].len = sizeof(req.header);
    bufs[0].device_write = false;

    bufs[1].addr = buffer;
    bufs[1].len = len;
    bufs[1].device_write = op == REQ_OP_READ;

    bufs[2].addr = &req.status;
    bufs[2].len = sizeof(req.status);
    bufs[2].device_write = true;

    /*
     * 当前是轮询式同步接口，一次只允许一个在途请求，
     * 避免并发调用者互相取走对方的 completion token。
     */
    mutex_lock(&blk->lock);

    ret = virtqueue_add(blk->vq, bufs, 3, &req);
    if (ret)
        goto out_unlock;

    virtqueue_kick(blk->vq);

    do {
        token = virtqueue_get_buf(blk->vq, &used_len);
    } while (!token);

    if (token != &req) {
        ret = -EIO;
        goto out_unlock;
    }

    switch (req.status) {
    case VIRTIO_BLK_S_OK:
        ret = 0;
        break;
    case VIRTIO_BLK_S_UNSUPP:
        ret = -EOPNOTSUPP;
        break;
    case VIRTIO_BLK_S_IOERR:
    default:
        ret = -EIO;
        break;
    }

out_unlock:
    mutex_unlock(&blk->lock);
    return ret;
}

static int virtio_blk_submit_bio(struct blkdev *bdev, struct bio *bio)
{
    struct request_queue *queue;
    struct virtio_blk *blk;
    struct virtio_blk_req req;
    struct virtio_buffer *bufs;
    unsigned int nr_bufs;
    unsigned int used_len;
    sector_t sector;
    u64 nr_sectors;
    void *token;
    int ret;

    if (!bdev || !bio || !bdev->bd_disk ||
        !bdev->bd_disk->queue)
        return -EINVAL;

    queue = bdev->bd_disk->queue;
    blk = queue->queuedata;
    if (!blk || !blk->vq)
        return -ENODEV;

    if (bio->op != REQ_OP_READ &&
        bio->op != REQ_OP_WRITE)
        return -EINVAL;

    if (!bio->bi_size ||
        bio->bi_size % SECTOR_SIZE != 0)
        return -EINVAL;

    //一个数据 vec 对应一个 descriptor,另外再加 header 和 status。
    nr_bufs = bio->bi_vcnt + 2;

    if (nr_bufs > blk->vq->num)
        return -EMSGSIZE;

    sector = bdev_sector_offset(bdev, bio->bi_sector);
    nr_sectors = bio->bi_size / SECTOR_SIZE;

    if (sector >= blk->capacity ||
        nr_sectors > blk->capacity - sector)
        return -EIO;

    bufs = kzalloc(sizeof(*bufs) * nr_bufs);
    if (!bufs)
        return -ENOMEM;

    req.header.type = bio->op == REQ_OP_READ ?
                      VIRTIO_BLK_T_IN :
                      VIRTIO_BLK_T_OUT;
    req.header.reserved = 0;
    req.header.sector = sector;
    req.status = 0xff;

    // 第一个 descriptor是请求头，设备读取。
    bufs[0].addr = &req.header;
    bufs[0].len = sizeof(req.header);
    bufs[0].device_write = false;

    // 中间 descriptors,BIO 中的所有数据段。
    for (unsigned int i = 0; i < bio->bi_vcnt; i++) {
        struct bio_vec *bvec = &bio->bi_io_vec[i];

        if (!bvec->page || !bvec->len) {
            ret = -EINVAL;
            goto out_free;
        }

        if (bvec->offset >= PAGE_SIZE ||
            bvec->len > PAGE_SIZE - bvec->offset) {
            ret = -EINVAL;
            goto out_free;
        }

        bufs[i + 1].addr =
            (u8 *)page_address(bvec->page) + bvec->offset;
        bufs[i + 1].len = bvec->len;


        bufs[i + 1].device_write = (bio->op == REQ_OP_READ);
    }

    // 最后一个 descriptor：状态字节，设备写入
    bufs[nr_bufs - 1].addr = &req.status;
    bufs[nr_bufs - 1].len = sizeof(req.status);
    bufs[nr_bufs - 1].device_write = true;

    // 当前还是同步轮询模型，只允许一个请求在途。因此栈上的 req 在完成前始终有效。
    mutex_lock(&blk->lock);

    ret = virtqueue_add(blk->vq, bufs, nr_bufs, &req);
    if (ret)
        goto out_unlock;

    virtqueue_kick(blk->vq);

    do {
        token = virtqueue_get_buf(blk->vq, &used_len);
    } while (!token);

    if (token != &req) {
        ret = -EIO;
        goto out_unlock;
    }

    switch (req.status) {
    case VIRTIO_BLK_S_OK:
        ret = 0;
        break;

    case VIRTIO_BLK_S_UNSUPP:
        ret = -EOPNOTSUPP;
        break;

    case VIRTIO_BLK_S_IOERR:
    default:
        ret = -EIO;
        break;
    }

out_unlock:
    mutex_unlock(&blk->lock);
out_free:
    kfree(bufs);
    return ret;
}

static struct block_device_operations virtio_blk_bdops = {
    .submit_bio = virtio_blk_submit_bio,
};

static int virtio_blk_probe(struct virtio_device *vdev)
{
    struct virtio_blk *blk;
    struct virtio_blk_config config;
    dev_t devnr;
    int ret;

    blk = kzalloc(sizeof(*blk));
    if (!blk)
        return -ENOMEM;

    blk->vdev = vdev;
    mutex_init(&blk->lock);
    blk->vq = virtqueue_create(vdev, 0, 128, NULL);
    if (!blk->vq) {
        printk("virtio-blk: failed to create virtqueue\n");
        kfree(blk);
        return -ENOMEM;
    }
    vdev->priv = blk;

    if (!vdev->config->get_config) {
        printk("virtio-blk: transport has no config accessor\n");
        return -ENODEV;
    }

    vdev->config->get_config(vdev, 0, &config, sizeof(config));
    blk->capacity = config.capacity;
    if (!blk->capacity) {
        printk("virtio-blk: zero or oversized capacity is unsupported\n");
        return -EOVERFLOW;
    }

    spin_lock_init(&blk->queue.lock);
    mutex_init(&blk->queue.rmw_lock);
    blk->queue.fops = &virtio_blk_bdops;
    blk->queue.logical_block_size = SECTOR_SIZE;
    blk->queue.max_hw_sectors = 1;
    blk->queue.queuedata = blk;

    strcpy(blk->disk.disk_name, "vda");
    blk->disk.capacity = (sector_t)blk->capacity;
    blk->disk.logical_block_size = SECTOR_SIZE;
    blk->disk.queue = &blk->queue;
    blk->disk.private_data = blk;

    ret = alloc_blkdev_region(&devnr, 1);
    if (ret)
        return ret;

    virtio_device_ready(vdev);

    ret = blkdev_register("vda", devnr, &blk->disk, NULL);
    if (ret) {
        printk("virtio-blk: block device registration failed: %d\n", ret);
        return ret;
    }

    printk("virtio-blk: registered vda, capacity=%xu sectors\n",
           (unsigned int)blk->capacity);
    return 0;
}

static int virtio_blk_remove(struct virtio_device *vdev)
{
    return 0;
}

static struct virtio_driver virtio_blk_driver = {
    .driver = {
        .name = "virtio-blk",
    },
    .id_table = virtio_blk_ids,
    .probe = virtio_blk_probe,
    .remove = virtio_blk_remove,
};

module_virtio_driver(virtio_blk_driver);
