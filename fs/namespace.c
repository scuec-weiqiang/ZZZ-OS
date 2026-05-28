#include <fs/dcache.h>
#include <fs/fs.h>
#include <fs/fs_context.h>
#include <fs/namei.h>
#include <fs/namespace.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/atomic.h>
#include <os/errno.h>
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
    spin_lock_init(&mnt->lock);
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

    init_fs.root.mnt = mntget(mnt);
    init_fs.root.dentry = dget(mnt->mnt_root);
    init_fs.pwd.mnt = mntget(mnt);
    init_fs.pwd.dentry = dget(mnt->mnt_root);

    list_add(&g_mounts, &mnt->mnt_list);
    return 0;
}

int vfs_mount_fs(const char *fs_name, const char *source, const char *target, u32 sb_flags) {
    struct file_system_type *fs_type = NULL;
    struct fs_context *fc = NULL;
    struct vfsmount *mnt = NULL;
    struct dentry *mountpoint = NULL;
    int ret = 0;

    CHECK(fs_name != NULL, "fs: invalid mount fs name", return -1;);
    CHECK(target != NULL, "fs: invalid mount target", return -1;);

    dprintk("mount_fs: fs=%s target=%s\n", fs_name, target);
    fs_type = get_fs_type(fs_name);
    CHECK(fs_type != NULL, "fs: mount fs type not found", return -1;);

    dprintk("mount_fs: lookup target %s\n", target);
    mountpoint = vfs_lookup(target);
    
    CHECK(mountpoint != NULL, "fs: mount target lookup failed", return -1;);
    CHECK(mountpoint->d_inode != NULL, "fs: mount target is negative dentry", dput(mountpoint); return -1;);
    CHECK(S_ISDIR(mountpoint->d_inode->i_mode), "fs: mount target is not a directory", dput(mountpoint); return -1;);
    CHECK(vfs_search_mount(mountpoint, &mnt) != 0, "fs: mount target already mounted", dput(mountpoint); return -1;);

    dprintk("mount_fs: alloc fs_context for %s\n", fs_name);
    fc = fs_context_for_mount(fs_type, sb_flags);
    CHECK(fc != NULL, "fs: alloc mount fs_context failed", dput(mountpoint); return -1;);

    fc->source = source;
    dprintk("mount_fs: get_tree for %s\n", fs_name);
    ret = vfs_get_tree(fc);
    CHECK(ret == 0, "fs: get_tree for mount failed",
          put_fs_context(fc);
          dput(mountpoint);
          return ret;);

    dprintk("mount_fs: kern_mount for %s\n", fs_name);
    ret = vfs_kern_mount(fc, &mnt);
    CHECK(ret == 0, "fs: create mount failed",
          put_fs_context(fc);
          dput(mountpoint);
          return ret;);

    mnt->mnt_mountpoint = dget(mountpoint);
    list_add_tail(&g_mounts, &mnt->mnt_list);
    dprintk("mount_fs: mounted %s on %s\n", fs_name, target);

    put_fs_context(fc);
    dput(mountpoint);
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

int vfs_get_mnt_parent(struct vfsmount *mnt, struct path *parent) {
    struct list_head *pos = NULL;
    struct dentry *mountpoint_parent = NULL;

    CHECK(mnt != NULL, "fs: invalid mount for parent lookup", return -EINVAL;);
    CHECK(parent != NULL, "fs: invalid parent path output", return -EINVAL;);

    if (mnt == g_root_mnt) {
        parent->mnt = mntget(g_root_mnt);
        parent->dentry = dget(g_root_mnt->mnt_root);
        return 0;
    }

    list_for_each(pos, &g_mounts) {
        struct vfsmount *iter = container_of(pos, struct vfsmount, mnt_list);

        if (iter == mnt) {
            continue;
        }

        if (iter->mnt_sb == mnt->mnt_mountpoint->d_sb) {
            mountpoint_parent = mnt->mnt_mountpoint->d_parent;
            if (mountpoint_parent == NULL) {
                mountpoint_parent = mnt->mnt_mountpoint;
            }
            parent->mnt = mntget(iter);
            parent->dentry = dget(mountpoint_parent);
            return 0;
        }
    }

    return -ENOENT;
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
