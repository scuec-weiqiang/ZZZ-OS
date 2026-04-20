#include <fs/file.h>
#include <fs/fs.h>
#include <fs/namei.h>
#include <fs/ramfs.h>
#include <os/check.h>
#include <os/printk.h>
#include <os/string.h>
#include <fs/dcache.h>

int fs2_selftest(void)
{
    static const char msg[] = "ramfs works";
    char buf[32];
    struct dentry *d = NULL;
    struct file *f = NULL;
    ssize_t n = 0;

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
    CHECK(f != NULL, "fs: open hello.txt for write failed", return -1;);
    n = kernel_write(f, msg, sizeof(msg) - 1);
    filp_close(f);
    CHECK(n == (ssize_t)(sizeof(msg) - 1), "fs: write hello.txt failed", return -1;);

    printk("$ cat /tmp/testdir/hello.txt\n");
    f = filp_open("/tmp/testdir/hello.txt", 0);
    CHECK(f != NULL, "fs: open hello.txt for read failed", return -1;);
    n = kernel_read(f, buf, sizeof(buf) - 1);
    filp_close(f);
    CHECK(n == (ssize_t)(sizeof(msg) - 1), "fs: read hello.txt failed", return -1;);
    CHECK(memcmp(buf, msg, sizeof(msg) - 1) == 0, "fs: content mismatch", return -1;);
    printk("%s\n", buf);

    printk("=========== pseudo shell demo ok ===========\n");
    printk("\n");
    return 0;
}
