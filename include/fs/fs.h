#ifndef FS2_FS_H
#define FS2_FS_H

#include <fs/types.h>
#include <fs/file.h>

int fs2_init(void);
int fs2_selftest(void);

int register_filesystem(struct file_system_type *fs);
struct file_system_type *get_fs_type(const char *name);

struct dentry *vfs_lookup(const char *path);
struct dentry *vfs_mkdir(const char *path, uint16_t mode);
struct dentry *vfs_create(const char *path, uint16_t mode);
struct dentry *vfs_mknod(const char *path, uint16_t mode, dev_t dev);
int do_execve(int fd, char *filename, char** argv, int flags);
#endif
