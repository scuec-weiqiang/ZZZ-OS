#ifndef FS2_FS_H
#define FS2_FS_H

#include <fs/types.h>
#include <fs/file.h>

int mount_root(const char *dev, const char *fs_type);
int fs2_selftest(void);

int register_filesystem(struct file_system_type *fs);
struct file_system_type *get_fs_type(const char *name);

int vfs_mount_fs(const char *fs_name, const char *source, const char *target, u32 sb_flags);
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
int do_execve(char *filename, char* argv[], char* envp[]);
#endif
