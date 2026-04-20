#ifndef FS2_NAMESPACE_H
#define FS2_NAMESPACE_H

#include <fs/types.h>

int vfs_kern_mount(struct fs_context *fc, struct vfsmount **mnt_out);
int vfs_kern_unmount(struct vfsmount *mnt);
int init_mount_tree(struct vfsmount *mnt);
int vfs_search_mount(struct dentry *dentry, struct vfsmount **mnt_out);
extern void mntput(struct vfsmount *mnt);
extern struct vfsmount *mntget(struct vfsmount *mnt);

#endif
