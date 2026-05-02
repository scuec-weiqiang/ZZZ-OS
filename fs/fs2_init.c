#include <fs/fs.h>
#include <fs/dcache.h>
#include <fs/fs_context.h>
#include <fs/inode.h>
#include <fs/namespace.h>
#include <fs/pagecache.h>
#include <fs/ramfs.h>
#include <fs/binfmt.h>
#include <os/devnode.h>
#include <os/init.h>
#include <os/check.h>

int mount_root(const char *dev, const char *fs_type)
{
    struct fs_context *fc = NULL;
    struct vfsmount *root_mnt = NULL;
    int ret = 0;

    ret = icache_init();
    CHECK(ret == 0, "fs: failed to init icache", return -1;);
    ret = dcache_init();
    CHECK(ret == 0, "fs: failed to init dcache", return -1;);
    ret = pagecache_init();
    CHECK(ret == 0, "fs: failed to init pagecache", return -1;);

    struct file_system_type *rootfs = get_fs_type(fs_type);
    CHECK(rootfs != NULL, "fs: ext2 filesystem is not registered", return -1;);

    fc = fs_context_for_mount(rootfs, 0);
    CHECK(fc != NULL, "fs: failed to alloc fs_context", return -1;);

    fc->source = dev;
    ret = vfs_get_tree(fc);
    if (ret < 0) {
        printk("failed to get tree for root fs:%d\n",ret);
    }
        
    ret = vfs_kern_mount(fc, &root_mnt);
    CHECK(ret == 0, "fs: failed to create root mount",
          put_fs_context(fc);
          return -1;);

    ret = init_mount_tree(root_mnt);
    CHECK(ret == 0, "fs: failed to install mount tree",
          put_fs_context(fc);
          return -1;);

    put_fs_context(fc);

    ret = vfs_mount_fs("ramfs", NULL, "/dev", 0);
    CHECK(ret == 0,
          "fs: failed to mount ramfs on /dev, make sure /dev exists in the ext2 rootfs image",
          return -1;);

    ret = devtmpfs_mount("/dev");
    CHECK(ret == 0, "fs: failed to populate /dev from devnodes", return -1;);

    elf_binfmt_init();

//     struct dentry *find = NULL;
//     find = vfs_lookup("/dev/uart0");

//     if (find != NULL) {
//         printk("found /dev/uart0\n");
//     } else {
//         printk("failed to find /dev/uart0\n");
//     }

//     struct file *f = filp_open("/dev/uart0", O_RDWR);
//     if (f != NULL) {
//         printk("opened /dev/uart0 successfully\n");
//     } else {
//         printk("failed to open /dev/uart0\n");
//     }
// 
//     kernel_write(f, "hello!\n", 7);
    return 0;
}
