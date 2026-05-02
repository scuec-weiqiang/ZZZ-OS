#include <fs/fs_context.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/string.h>

/* 给某个 file_system_type 分配一个 fs_context */
struct fs_context *fs_context_for_mount(struct file_system_type *fs_type, u32 sb_flags) {
    struct fs_context *fc = NULL;
    int ret = 0;

    CHECK(fs_type != NULL, "fs: invalid fs_type", return NULL;);

    fc = kmalloc(sizeof(*fc));
    CHECK(fc != NULL, "fs: alloc fs_context failed", return NULL;);
    memset(fc, 0, sizeof(*fc));

    fc->fs_type = fs_type;
    fc->sb_flags = sb_flags;
    fc->purpose = FS_CONTEXT_FOR_MOUNT;

    if (fs_type->init_fs_context != NULL) {
        ret = fs_type->init_fs_context(fc);
        CHECK(ret == 0, "fs: init_fs_context failed", kfree(fc); return NULL;);
    }

    return fc;
}

void put_fs_context(struct fs_context *fc) {
    if (fc != NULL) {
        kfree(fc);
    }
}

int vfs_get_tree(struct fs_context *fc) {
    CHECK(fc != NULL, "fs: invalid fs_context", return -1;);
    CHECK(fc->fs_type != NULL, "fs: fs_context missing fs_type", return -1;);
    CHECK(fc->fs_type->get_tree != NULL, "fs: filesystem missing get_tree", return -1;);

    return fc->fs_type->get_tree(fc);
}
