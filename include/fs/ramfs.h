#ifndef FS2_RAMFS_H
#define FS2_RAMFS_H

#include <fs/types.h>

extern struct file_system_type ramfs_fs_type;
int ramfs_debug_ls(const char *path);

#endif
