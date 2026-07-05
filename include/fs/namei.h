#ifndef FS2_NAMEI_H
#define FS2_NAMEI_H

#include <fs/types.h>

int path_lookup(const char *path, struct path *out);
int path_lookup_nofollow(const char *path, struct path *out);
int path_parentat(const char *path, struct path *parent, struct qstr *last);
struct dentry *vfs_lookup(const char *path);
int vfs_mkdir(const char *path, u16 mode);
struct dentry *vfs_create(const char *path, u16 mode);
struct dentry *vfs_mknod(const char *path, u16 mode, dev_t dev);
int vfs_unlink(const char *path);
int vfs_rmdir(const char *path);
int vfs_rename(const char *old_path, const char *new_path);
struct dentry *vfs_symlink(const char *path, const char *target);
ssize_t vfs_readlink(const char *path, char *buf, size_t size);
int vfs_access(const char *path, int mode);
int vfs_chdir(const char *path);
int vfs_getcwd(char *buf, size_t size);

void path_get(const struct path *path);
void path_put(const struct path *path);

#endif
