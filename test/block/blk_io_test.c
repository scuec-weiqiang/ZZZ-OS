/*
 * ZZZ-OS read-only block I/O correctness test.
 * Compares unaligned and cross-page reads with one aligned reference read.
 */

#include <fs/blkdev.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/timekeeping.h>
#include <os/types.h>

#define BLK_IO_TEST_DEVICE         "vda"
#define BLK_IO_TEST_REFERENCE_SIZE (128U * 1024U)
#define BLK_IO_TEST_ARENA_PAGES    20U
#define BLK_IO_TEST_GUARD_SIZE     32U

static u64 blk_io_checksum(const u8 *data, size_t len)
{
    u64 hash = 1469598103934665603ULL;

    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static int blk_io_guard_valid(const u8 *guard, size_t len, u8 value)
{
    for (size_t i = 0; i < len; i++) {
        if (guard[i] != value)
            return 0;
    }
    return 1;
}

static int blk_io_test_init(void)
{
    static const size_t buffer_offsets[] = { 0, 1, 511, 512, 4095 };
    static const u64 disk_positions[] = {
        0, 1, 511, 512, 513, 4095, 4096, 4097, 65521
    };
    static const size_t lengths[] = {
        1, 511, 512, 513, 4095, 4096, 4097, 65536
    };
    struct blkdev *bdev;
    u8 *reference = NULL;
    u8 *arena = NULL;
    u64 start_ns;
    u64 verified_bytes = 0;
    u64 cases = 0;
    u64 checksum;
    int ret = 0;

    printk("blk-io-test: START device=%s reference_bytes=%u read_only=1\n",
           BLK_IO_TEST_DEVICE, BLK_IO_TEST_REFERENCE_SIZE);
    start_ns = monotonic_ns();

    bdev = blkdev_get_by_path(BLK_IO_TEST_DEVICE);
    if (IS_ERR(bdev))
        return PTR_ERR(bdev);

    reference = page_alloc(BLK_IO_TEST_REFERENCE_SIZE / PAGE_SIZE);
    arena = page_alloc(BLK_IO_TEST_ARENA_PAGES);
    if (!reference || !arena) {
        ret = -ENOMEM;
        goto out;
    }

    ret = blkdev_read(bdev, reference, BLK_IO_TEST_REFERENCE_SIZE, 0);
    if (ret < 0)
        goto out;
    checksum = blk_io_checksum(reference, BLK_IO_TEST_REFERENCE_SIZE);

    for (u32 oi = 0;
         oi < sizeof(buffer_offsets) / sizeof(buffer_offsets[0]); oi++) {
        for (u32 pi = 0;
             pi < sizeof(disk_positions) / sizeof(disk_positions[0]); pi++) {
            for (u32 li = 0; li < sizeof(lengths) / sizeof(lengths[0]); li++) {
                size_t buffer_offset = buffer_offsets[oi];
                u64 disk_position = disk_positions[pi];
                size_t length = lengths[li];
                u8 *buffer;

                if (disk_position + length > BLK_IO_TEST_REFERENCE_SIZE)
                    continue;

                buffer = arena + PAGE_SIZE + buffer_offset;
                memset(buffer - BLK_IO_TEST_GUARD_SIZE, 0xa5,
                       BLK_IO_TEST_GUARD_SIZE);
                memset(buffer + length, 0x5a, BLK_IO_TEST_GUARD_SIZE);

                ret = blkdev_read(bdev, buffer, length, disk_position);
                if (ret < 0) {
                    printk("blk-io-test: FAIL read case=%lu pos=%lu len=%lu "
                           "buffer_offset=%lu ret=%d\n",
                           (unsigned long)cases, (unsigned long)disk_position,
                           (unsigned long)length,
                           (unsigned long)buffer_offset, ret);
                    goto out;
                }

                if (memcmp(buffer, reference + disk_position, length) != 0) {
                    size_t mismatch = 0;

                    while (mismatch < length &&
                           buffer[mismatch] == reference[disk_position + mismatch])
                        mismatch++;
                    printk("blk-io-test: FAIL data case=%lu pos=%lu len=%lu "
                           "buffer_offset=%lu mismatch=%lu expected=%u actual=%u\n",
                           (unsigned long)cases, (unsigned long)disk_position,
                           (unsigned long)length, (unsigned long)buffer_offset,
                           (unsigned long)mismatch,
                           (unsigned int)reference[disk_position + mismatch],
                           (unsigned int)buffer[mismatch]);
                    ret = -EIO;
                    goto out;
                }

                if (!blk_io_guard_valid(buffer - BLK_IO_TEST_GUARD_SIZE,
                                        BLK_IO_TEST_GUARD_SIZE, 0xa5) ||
                    !blk_io_guard_valid(buffer + length,
                                        BLK_IO_TEST_GUARD_SIZE, 0x5a)) {
                    printk("blk-io-test: FAIL guard case=%lu pos=%lu len=%lu "
                           "buffer_offset=%lu\n",
                           (unsigned long)cases, (unsigned long)disk_position,
                           (unsigned long)length,
                           (unsigned long)buffer_offset);
                    ret = -EIO;
                    goto out;
                }

                cases++;
                verified_bytes += length;
            }
        }
    }

out:
    page_free(arena);
    page_free(reference);
    blkdev_put(bdev);

    if (ret < 0) {
        printk("blk-io-test: FAIL ret=%d cases=%lu verified_bytes=%lu "
               "time_us=%lu\n",
               ret, (unsigned long)cases, (unsigned long)verified_bytes,
               (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
        return ret;
    }

    printk("blk-io-test: PASS cases=%lu verified_bytes=%lu checksum=%lx "
           "time_us=%lu\n",
           (unsigned long)cases, (unsigned long)verified_bytes,
           (unsigned long)checksum,
           (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
    return 0;
}

late_initcall(blk_io_test_init);
