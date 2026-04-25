#include <fs/blkdev.h>
#include <fs/file.h>
#include <fs/fs.h>
#include <fs/namei.h>
#include <fs/ramfs.h>
#include <os/errno.h>
#include <os/check.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/string.h>
#include <fs/dcache.h>

static void blkdev_print_text_sample(const char *tag, const uint8_t *buf, size_t len)
{
    size_t n = 0;
    char line[160];

    if (buf == NULL) {
        return;
    }

    if (len > sizeof(line) - 1) {
        len = sizeof(line) - 1;
    }

    for (size_t i = 0; i < len; i++) {
        char c = (char)buf[i];

        if (c == '\n' || c == '\r' || c == '\t') {
            c = ' ';
        }
        if (c < 32 || c > 126) {
            c = '.';
        }
        line[n++] = c;
    }
    line[n] = '\0';
    printk("%s%s\n", tag, line);
}

static int blkdev_selftest(void)
{
    struct block_device *bdev = NULL;
    uint8_t *expected = NULL;
    uint8_t *readback = NULL;
    uint8_t *patch = NULL;
    uint8_t *text_window = NULL;
    uint8_t *text_readback = NULL;
    uint32_t sector_size = 0;
    size_t total_len = 0;
    size_t patch_pos = 0;
    size_t patch_len = 0;
    size_t text_window_len = 0;
    size_t text_pos = 0;
    size_t text_len = 0;
    static const char text_payload[] =
        "blkdev non-aligned text payload: hello from ram_disk, crossing sectors on purpose.";
    int ret = 0;

    printk("\n");
    printk("=========== blkdev selftest on ram_disk ===========\n");

    bdev = blkdev_get_by_path("ram_disk");
    CHECK(!IS_ERR(bdev), "blkdev: open ram_disk failed", return PTR_ERR(bdev););

    sector_size = bdev->bd_disk->logical_block_size;
    total_len = PAGE_SIZE + sector_size + 137;
    patch_pos = 37;
    patch_len = total_len - patch_pos;
    text_pos = sector_size - 19;
    text_len = sizeof(text_payload) - 1;
    text_window_len = text_pos + text_len + 23;

    expected = kmalloc(total_len);
    readback = kmalloc(total_len);
    patch = kmalloc(patch_len);
    text_window = kmalloc(text_window_len);
    text_readback = kmalloc(text_len + 1);
    CHECK(expected != NULL && readback != NULL && patch != NULL &&
          text_window != NULL && text_readback != NULL,
          "blkdev: alloc selftest buffers failed",
          ret = -ENOMEM;
          goto out;);

    memset(expected, 0x11, total_len);
    for (size_t i = 0; i < patch_len; i++) {
        patch[i] = (uint8_t)(i * 37U + 13U);
    }
    memcpy(expected + patch_pos, patch, patch_len);
    memset(readback, 0, total_len);

    printk("blkdev-test: aligned write len=%xu pos=0\n", (unsigned)total_len);
    ret = blkdev_write(bdev, expected, total_len, 0);
    CHECK(ret == 0, "blkdev: baseline write failed", goto out;);

    printk("blkdev-test: partial write len=%xu pos=%xu\n",
           (unsigned)patch_len, (unsigned)patch_pos);
    ret = blkdev_write(bdev, patch, patch_len, patch_pos);
    CHECK(ret == 0, "blkdev: partial write failed", goto out;);

    printk("blkdev-test: read back len=%xu pos=0\n", (unsigned)total_len);
    ret = blkdev_read(bdev, readback, total_len, 0);
    CHECK(ret == 0, "blkdev: read back failed", goto out;);

    CHECK(memcmp(readback, expected, total_len) == 0,
          "blkdev: read data mismatch", ret = -EIO; goto out;);

    memset(text_window, '.', text_window_len);
    memcpy(text_window + text_pos, text_payload, text_len);
    memset(text_readback, 0, text_len + 1);

    printk("blkdev-test: text write len=%xu pos=%xu sector=%xu\n",
           (unsigned)text_len, (unsigned)text_pos, (unsigned)sector_size);
    ret = blkdev_write(bdev, text_window + text_pos, text_len, text_pos);
    CHECK(ret == 0, "blkdev: text write failed", goto out;);

    printk("blkdev-test: text read len=%xu pos=%xu\n",
           (unsigned)text_len, (unsigned)text_pos);
    ret = blkdev_read(bdev, text_readback, text_len, text_pos);
    CHECK(ret == 0, "blkdev: text read failed", goto out;);
    CHECK(memcmp(text_readback, text_payload, text_len) == 0,
          "blkdev: text payload mismatch", ret = -EIO; goto out;);

    printk("blkdev-test: text window read len=%xu pos=0\n", (unsigned)text_window_len);
    ret = blkdev_read(bdev, text_window, text_window_len, 0);
    CHECK(ret == 0, "blkdev: text window read failed", goto out;);

    blkdev_print_text_sample("blkdev-test: expected text: ", (const uint8_t *)text_payload, text_len);
    blkdev_print_text_sample("blkdev-test: readback text: ", text_readback, text_len);
    blkdev_print_text_sample("blkdev-test: disk prefix    : ", text_window, text_window_len);

    printk("=========== blkdev selftest ok ===========\n");
    printk("\n");
    ret = 0;

out:
    if (text_readback != NULL) {
        kfree(text_readback);
    }
    if (text_window != NULL) {
        kfree(text_window);
    }
    if (patch != NULL) {
        kfree(patch);
    }
    if (readback != NULL) {
        kfree(readback);
    }
    if (expected != NULL) {
        kfree(expected);
    }
    if (!IS_ERR_OR_NULL(bdev)) {
        blkdev_put(bdev);
    }
    return ret;
}

int fs2_selftest(void)
{
    static const char msg[] = "ramfs works";
    char buf[32];
    struct dentry *d = NULL;
    struct file *f = NULL;
    ssize_t n = 0;

    // n = blkdev_selftest();
    // CHECK(n == 0, "fs: blkdev selftest failed", return (int)n;);

    memset(buf, 0, sizeof(buf));

    printk("\n");
    printk("=========== pseudo shell on ramfs ===========\n");
    printk("$ mkdir /tmp\n");
    d = vfs_mkdir("/tmp", 0755);
    CHECK(d != NULL, "fs: mkdir /tmp failed", return -1;);
    dput(d);

    printk("$ mkdir /tmp/testdir\n");
    d = vfs_mkdir("/tmp/testdir", 0755);
    CHECK(d != NULL, "fs: mkdir /tmp/testdir failed", return -1;);
    dput(d);

    printk("$ create /tmp/testdir/hello.txt\n");
    d = vfs_create("/tmp/testdir/hello.txt", 0644);
    CHECK(d != NULL, "fs: create hello.txt failed", return -1;);
    dput(d);

    printk("$ ls /\n");
    CHECK(ramfs_debug_ls("/") == 0, "fs: ls / failed", return -1;);

    printk("$ ls /tmp\n");
    CHECK(ramfs_debug_ls("/tmp") == 0, "fs: ls /tmp failed", return -1;);

    printk("$ ls /tmp/testdir\n");
    CHECK(ramfs_debug_ls("/tmp/testdir") == 0, "fs: ls /tmp/testdir failed", return -1;);

    printk("$ echo \"%s\" > /tmp/testdir/hello.txt\n", msg);
    f = filp_open("/tmp/testdir/hello.txt", 0);
    CHECK(!IS_ERR(f), "fs: open hello.txt for write failed", return -1;);
    n = kernel_write(f, msg, sizeof(msg) - 1);
    filp_close(f);
    CHECK(n == (ssize_t)(sizeof(msg) - 1), "fs: write hello.txt failed", return -1;);

    printk("$ cat /tmp/testdir/hello.txt\n");
    f = filp_open("/tmp/testdir/hello.txt", 0);
    CHECK(!IS_ERR(f), "fs: open hello.txt for read failed", return -1;);
    n = kernel_read(f, buf, sizeof(buf) - 1);
    filp_close(f);
    CHECK(n == (ssize_t)(sizeof(msg) - 1), "fs: read hello.txt failed", return -1;);
    CHECK(memcmp(buf, msg, sizeof(msg) - 1) == 0, "fs: content mismatch", return -1;);
    printk("%s\n", buf);

    printk("=========== pseudo shell demo ok ===========\n");
    printk("\n");
    return 0;
}
