#ifndef FS2_SUPER_H
#define FS2_SUPER_H

#include <fs/types.h>

struct super_block *alloc_super(struct file_system_type *type);
void destroy_super(struct super_block *sb);

#endif
