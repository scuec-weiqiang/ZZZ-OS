#ifndef FS2_NAMEI_H
#define FS2_NAMEI_H

#include <fs/types.h>

int path_lookup(const char *path, struct path *out);
int path_parentat(const char *path, struct path *parent, struct qstr *last);
struct dentry *vfs_lookup(const char *path);
struct dentry *vfs_mkdir(const char *path, u16 mode);
struct dentry *vfs_create(const char *path, u16 mode);
struct dentry *vfs_mknod(const char *path, u16 mode, dev_t dev);
int vfs_chdir(const char *path);
int vfs_getcwd(char *buf, size_t size);

void path_get(const struct path *path);
void path_put(const struct path *path);

#endif
