#include <fs/dcache.h>
#include <fs/namespace.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/atomic.h>
#include <fs/fs_struct.h>

static LIST_HEAD(g_mounts);
static struct vfsmount *g_root_mnt;

/* 根据 fs_context 创建一个新的 vfsmount */
int vfs_kern_mount(struct fs_context *fc, struct vfsmount **mnt_out) {
    struct vfsmount *mnt = NULL;

    CHECK(fc != NULL, "fs: invalid fs_context for mount", return -1;);
    CHECK(fc->root != NULL, "fs: fs_context has no root", return -1;);
    CHECK(mnt_out != NULL, "fs: invalid mount output", return -1;);

    mnt = kmalloc(sizeof(*mnt));
    CHECK(mnt != NULL, "fs: alloc vfsmount failed", return -1;);

    mnt->mnt_sb = fc->root->d_sb;
    mnt->mnt_root = dget(fc->root);
    mnt->mnt_mountpoint = mnt->mnt_root;
    mnt->refcount = 1;
    INIT_LIST_HEAD(&mnt->mnt_list);

    *mnt_out = mnt;
    return 0;
}

/* 将根挂载点初始化为新建的挂载点 */
int init_mount_tree(struct vfsmount *mnt) {
    CHECK(mnt != NULL, "fs: invalid root mount", return -1;);
    CHECK(g_root_mnt == NULL, "fs: mount tree already initialized", return -1;);

    g_root_mnt = mnt;
    
    init_fs.root = (struct path){.mnt = mnt, .dentry = mnt->mnt_root};
    init_fs.pwd = init_fs.root;

    list_add(&g_mounts, &mnt->mnt_list);
    return 0;
}

int vfs_kern_unmount(struct vfsmount *mnt) {
    CHECK(mnt != NULL, "fs: invalid mount point", return -1;);
    CHECK(mnt == g_root_mnt, "fs: only root mount can be unmounted", return -1;);

    g_root_mnt = NULL;
    list_del(&mnt->mnt_list);
    kfree(mnt);
    return 0;
}

int vfs_search_mount(struct dentry *dentry, struct vfsmount **mnt_out) {
    struct list_head *pos = NULL;

    CHECK(dentry != NULL, "fs: invalid dentry for mount search", return -1;);
    CHECK(mnt_out != NULL, "fs: invalid mount output", return -1;);

    list_for_each(pos, &g_mounts) {
        struct vfsmount *mnt = container_of(pos, struct vfsmount, mnt_list);
        if (mnt->mnt_mountpoint == dentry) {
            *mnt_out = mnt;
            return 0;
        }
    }

    return -1;
}

void mntput(struct vfsmount *mnt) {
    if (mnt == NULL)
        return;

    spin_lock(&mnt->lock);
    mnt->refcount --;
    if (mnt->refcount == 0) {
        spin_unlock(&mnt->lock);
        kfree(mnt);
        return;
    }
    spin_unlock(&mnt->lock);
}

struct vfsmount *mntget(struct vfsmount *mnt) {
    if (mnt == NULL)
        return NULL;

    spin_lock(&mnt->lock);
    mnt->refcount++;
    spin_unlock(&mnt->lock);
    return mnt;
}
