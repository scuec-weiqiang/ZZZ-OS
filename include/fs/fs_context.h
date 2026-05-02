#ifndef FS2_FS_CONTEXT_H
#define FS2_FS_CONTEXT_H

#include <fs/types.h>

enum fs_context_purpose {
    FS_CONTEXT_FOR_MOUNT = 1,
};

struct fs_context *fs_context_for_mount(struct file_system_type *fs_type, u32 sb_flags);
void put_fs_context(struct fs_context *fc);
int vfs_get_tree(struct fs_context *fc);

#endif
