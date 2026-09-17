/*
 * ZZZ-OS read-only VFS/Ext2/page-cache correctness test.
 * Every fragmented and random read is compared with a baseline image.
 */

#include <fs/file.h>
#include <os/err.h>
#include <os/errno.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/timekeeping.h>
#include <os/types.h>
#include <uapi/fcntl_defs.h>

#define FS_READ_TEST_PATH        "/bin/ls"
#define FS_READ_TEST_MAX_BYTES   (2U * 1024U * 1024U)
#define FS_READ_TEST_BUFFER_SIZE (64U * 1024U)
#define FS_READ_TEST_RANDOM_CASES 128U
#define FS_READ_TEST_REOPENS      16U

static u64 fs_read_checksum(const u8 *data, size_t len)
{
    u64 hash = 1469598103934665603ULL;

    for (size_t i = 0; i < len; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static int fs_read_exact_at(struct file *file, u8 *buffer,
                            size_t length, size_t position)
{
    size_t done = 0;

    while (done < length) {
        ssize_t ret = kernel_read_at(file, (loff_t)(position + done),
                                     (char *)buffer + done, length - done);

        if (ret < 0)
            return (int)ret;
        if (ret == 0)
            return -EIO;
        done += (size_t)ret;
    }
    return 0;
}

static int fs_read_compare(struct file *file, const u8 *baseline,
                           u8 *buffer, size_t position, size_t length,
                           u64 case_no)
{
    int ret = fs_read_exact_at(file, buffer, length, position);

    if (ret < 0) {
        printk("fs-read-test: FAIL read case=%lu pos=%lu len=%lu ret=%d\n",
               (unsigned long)case_no, (unsigned long)position,
               (unsigned long)length, ret);
        return ret;
    }

    if (memcmp(buffer, baseline + position, length) != 0) {
        size_t mismatch = 0;

        while (mismatch < length &&
               buffer[mismatch] == baseline[position + mismatch])
            mismatch++;
        printk("fs-read-test: FAIL data case=%lu pos=%lu len=%lu "
               "mismatch=%lu expected=%u actual=%u\n",
               (unsigned long)case_no, (unsigned long)position,
               (unsigned long)length, (unsigned long)mismatch,
               (unsigned int)baseline[position + mismatch],
               (unsigned int)buffer[mismatch]);
        return -EIO;
    }
    return 0;
}

static int fs_read_test_init(void)
{
    static const size_t chunk_sizes[] = {
        1, 511, 512, 513, 1024, 4095, 4096, 4097, 65536
    };
    struct file *file;
    u8 *baseline = NULL;
    u8 *buffer = NULL;
    size_t test_bytes;
    u64 cases = 0;
    u64 verified_bytes = 0;
    u64 start_ns = monotonic_ns();
    u64 checksum = 0;
    u32 random_state = 0x5a17c3e1U;
    int ret = 0;

    printk("fs-read-test: START file=%s read_only=1 seed=%u\n",
           FS_READ_TEST_PATH, random_state);

    file = filp_open(FS_READ_TEST_PATH, O_RDONLY);
    if (IS_ERR(file))
        return PTR_ERR(file);

    test_bytes = file->f_inode->i_size;
    if (test_bytes > FS_READ_TEST_MAX_BYTES)
        test_bytes = FS_READ_TEST_MAX_BYTES;
    if (test_bytes == 0) {
        ret = -EINVAL;
        goto out_close;
    }

    baseline = page_alloc((test_bytes + PAGE_SIZE - 1) / PAGE_SIZE);
    buffer = page_alloc(FS_READ_TEST_BUFFER_SIZE / PAGE_SIZE);
    if (!baseline || !buffer) {
        ret = -ENOMEM;
        goto out;
    }

    ret = fs_read_exact_at(file, baseline, test_bytes, 0);
    if (ret < 0)
        goto out;
    checksum = fs_read_checksum(baseline, test_bytes);

    for (u32 ci = 0; ci < sizeof(chunk_sizes) / sizeof(chunk_sizes[0]); ci++) {
        size_t chunk = chunk_sizes[ci];
        size_t limit = test_bytes;

        /* Keep tiny-read stress useful without adding millions of calls. */
        if (chunk == 1 && limit > 8192)
            limit = 8192;
        else if (chunk < 512 && limit > 65536)
            limit = 65536;

        for (size_t position = 0; position < limit; position += chunk) {
            size_t length = chunk;

            if (length > limit - position)
                length = limit - position;
            ret = fs_read_compare(file, baseline, buffer, position, length,
                                  cases);
            if (ret < 0)
                goto out;
            cases++;
            verified_bytes += length;
        }
    }

    /* Probe both direct->single and single->double indirect boundaries. */
    {
        size_t block_size = file->f_inode->i_sb->s_blocksize;
        size_t boundaries[2];

        boundaries[0] = 12U * block_size;
        boundaries[1] = (12U + block_size / sizeof(u32)) * block_size;
        for (u32 i = 0; i < 2; i++) {
            size_t position;
            size_t length;

            if (boundaries[i] >= test_bytes)
                continue;
            position = boundaries[i] > 33 ? boundaries[i] - 33 : 0;
            length = test_bytes - position;
            if (length > 129)
                length = 129;
            ret = fs_read_compare(file, baseline, buffer, position, length,
                                  cases);
            if (ret < 0)
                goto out;
            cases++;
            verified_bytes += length;
        }
    }

    for (u32 i = 0; i < FS_READ_TEST_RANDOM_CASES; i++) {
        size_t length;
        size_t position;

        random_state = random_state * 1664525U + 1013904223U;
        length = (random_state % 8192U) + 1U;
        if (length > test_bytes)
            length = test_bytes;
        random_state = random_state * 1664525U + 1013904223U;
        position = random_state % (test_bytes - length + 1);

        ret = fs_read_compare(file, baseline, buffer, position, length,
                              cases);
        if (ret < 0)
            goto out;
        cases++;
        verified_bytes += length;
    }

    for (u32 i = 0; i < FS_READ_TEST_REOPENS; i++) {
        struct file *reopened = filp_open(FS_READ_TEST_PATH, O_RDONLY);
        size_t length = test_bytes > 512 ? 512 : test_bytes;

        if (IS_ERR(reopened)) {
            ret = PTR_ERR(reopened);
            goto out;
        }
        ret = fs_read_compare(reopened, baseline, buffer, 0, length, cases);
        filp_close(reopened);
        if (ret < 0)
            goto out;
        cases++;
        verified_bytes += length;
    }

out:
    page_free(buffer);
    page_free(baseline);
out_close:
    filp_close(file);

    if (ret < 0) {
        printk("fs-read-test: FAIL ret=%d cases=%lu verified_bytes=%lu "
               "time_us=%lu\n",
               ret, (unsigned long)cases, (unsigned long)verified_bytes,
               (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
        return ret;
    }

    printk("fs-read-test: PASS cases=%lu source_bytes=%lu "
           "verified_bytes=%lu checksum=%lx reopens=%u time_us=%lu\n",
           (unsigned long)cases, (unsigned long)test_bytes,
           (unsigned long)verified_bytes, (unsigned long)checksum,
           FS_READ_TEST_REOPENS,
           (unsigned long)((monotonic_ns() - start_ns) / 1000ULL));
    return 0;
}

late_initcall(fs_read_test_init);
