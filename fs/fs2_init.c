#include <fs/fs.h>
#include <fs/dcache.h>
#include <fs/fs_context.h>
#include <fs/inode.h>
#include <fs/namespace.h>
#include <fs/pagecache.h>
#include <fs/ramfs.h>
#include <fs/binfmt.h>
#include <os/check.h>
extern struct file_system_type ext2_fs_type;
int fs2_init(void)
{
//     struct fs_context *fc = NULL;
//     struct vfsmount *root_mnt = NULL;
//     int ret = 0;

//     ret = icache_init();
//     CHECK(ret == 0, "fs: failed to init icache", return -1;);

//     ret = dcache_init();
//     CHECK(ret == 0, "fs: failed to init dcache", return -1;);

//     ret = pagecache_init();
//     CHECK(ret == 0, "fs: failed to init pagecache", return -1;);

//     ret = register_filesystem(&ramfs_fs_type);
//     CHECK(ret == 0, "fs: failed to register ramfs", return -1;);

//     fc = fs_context_for_mount(&ramfs_fs_type, 0);
//     CHECK(fc != NULL, "fs: failed to alloc fs_context", return -1;);

//     ret = vfs_get_tree(fc);
//     CHECK(ret == 0, "fs: failed to build root tree",
//           put_fs_context(fc);
//           return -1;);

//     ret = vfs_kern_mount(fc, &root_mnt);
//     CHECK(ret == 0, "fs: failed to create root mount",
//           put_fs_context(fc);
//           return -1;);

//     ret = init_mount_tree(root_mnt);
//     CHECK(ret == 0, "fs: failed to install mount tree",
//           put_fs_context(fc);
//           return -1;);

//     put_fs_context(fc);
    // ret = fs2_selftest();
    // CHECK(ret == 0, "fs: selftest failed", return -1;);


    struct fs_context *fc = NULL;
    struct vfsmount *root_mnt = NULL;
    int ret = 0;

    ret = icache_init();
    CHECK(ret == 0, "fs: failed to init icache", return -1;);

    ret = dcache_init();
    CHECK(ret == 0, "fs: failed to init dcache", return -1;);

    ret = pagecache_init();
    CHECK(ret == 0, "fs: failed to init pagecache", return -1;);

    ret = register_filesystem(&ext2_fs_type);
    CHECK(ret == 0, "fs: failed to register ramfs", return -1;);

    fc = fs_context_for_mount(&ext2_fs_type, 0);
    CHECK(fc != NULL, "fs: failed to alloc fs_context", return -1;);
    fc->source = "ram_disk";
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

    elf_binfmt_init();

    struct dentry *find = NULL;

    find = vfs_lookup("/proc1");
    if (find) {
        printk("lookup /proc1, found dentry %s, inode = %xu, size = %xu\n", find->d_name.name, find->d_inode->i_ino, find->d_inode->i_size);
    }

    char buf[64];
    extern void *memset(void *s, int c, size_t n);
    memset(buf, 0, sizeof(buf));
    struct file *fp = filp_open("/hello.txt", O_RDONLY);
    if (fp) {
        ssize_t bytes = kernel_read_at(fp, 0, buf, 64);
        if (bytes > 0) {
            printk("read %d bytes from /hello.txt: %s\n", bytes, (char*)buf);
            here;
        } else {    
            printk("failed to read from /hello.txt, ret=%d\n", bytes);
        }
        filp_close(fp);
    } else {
        printk("failed to open /hello.txt\n");
    }



    return 0;
}
