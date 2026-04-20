#include <fs/fs.h>
#include <fs/dcache.h>
#include <fs/fs_context.h>
#include <fs/inode.h>
#include <fs/namespace.h>
#include <fs/ramfs.h>
#include <os/check.h>

int fs2_init(void)
{
    struct fs_context *fc = NULL;
    struct vfsmount *root_mnt = NULL;
    int ret = 0;

    ret = icache_init();
    CHECK(ret == 0, "fs: failed to init icache", return -1;);

    ret = dcache_init();
    CHECK(ret == 0, "fs: failed to init dcache", return -1;);

    ret = register_filesystem(&ramfs_fs_type);
    CHECK(ret == 0, "fs: failed to register ramfs", return -1;);

    fc = fs_context_for_mount(&ramfs_fs_type, 0);
    CHECK(fc != NULL, "fs: failed to alloc fs_context", return -1;);

    ret = vfs_get_tree(fc);
    CHECK(ret == 0, "fs: failed to build root tree",
          put_fs_context(fc);
          return -1;);

    ret = vfs_kern_mount(fc, &root_mnt);
    CHECK(ret == 0, "fs: failed to create root mount",
          put_fs_context(fc);
          return -1;);

    ret = init_mount_tree(root_mnt);
    CHECK(ret == 0, "fs: failed to install mount tree",
          put_fs_context(fc);
          return -1;);

    put_fs_context(fc);
    ret = fs2_selftest();
    CHECK(ret == 0, "fs: selftest failed", return -1;);
    return 0;
}
