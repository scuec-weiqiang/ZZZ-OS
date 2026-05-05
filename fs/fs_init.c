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

int mount_root(const char *dev, const char *fs_type) {
    struct fs_context *fc = NULL;
    struct vfsmount *root_mnt = NULL;
    int ret = 0;

    ret = icache_init();
    if (ret < 0) {
        panic("fs: failed to init icache");
    }

    ret = dcache_init();
    if (ret < 0) {
        panic("fs: failed to init dcache");
    }

    ret = pagecache_init();
    if (ret < 0) {
        panic("fs: failed to init pagecache");
    }

    struct file_system_type *rootfs = get_fs_type(fs_type);
    if (!rootfs) {
        panic("fs: root fs type not found");
    }

    fc = fs_context_for_mount(rootfs, 0);
    if (!fc) {
        panic("fs: failed to create fs context for root fs");
    }

    fc->source = dev;
    ret = vfs_get_tree(fc);
    if (ret < 0) {
        panic("failed to get tree for root fs:%d\n",ret);
    }
        
    ret = vfs_kern_mount(fc, &root_mnt);
    if (ret < 0) {
        put_fs_context(fc);
        panic("fs: failed to mount root fs");
    }
 

    ret = init_mount_tree(root_mnt);
    if(ret< 0) {
          put_fs_context(fc);
          panic("fs: failed to install mount tree");
    }

    put_fs_context(fc);

    ret = vfs_mount_fs("ramfs", NULL, "/dev", 0);
    if (ret < 0) {
        panic("fs: failed to mount ramfs on /dev, make sure /dev exists in the ext2 rootfs image");
    }

    ret = devtmpfs_mount("/dev");
    if (ret < 0) {
        panic("fs: failed to populate /dev from devnodes");
    }

    elf_binfmt_init();
    return 0;
}
