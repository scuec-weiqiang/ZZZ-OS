#ifndef _OS_FS_STRUCT_H
#define _OS_FS_STRUCT_H

#include <fs/namei.h>
#include <os/spinlock.h>

extern void set_fs_root(struct fs_struct *, const struct path *);
extern void set_fs_pwd(struct fs_struct *, const struct path *);
extern struct fs_struct *copy_fs_struct(struct fs_struct *);
extern void free_fs_struct(struct fs_struct *);
extern int unshare_fs_struct(void);

int alloc_fs_struct_init(void);
struct fs_struct *alloc_fs_struct(void);
void free_fs_struct(struct fs_struct *fs);

struct fs_struct *get_fs_struct(struct fs_struct *fs);
void put_fs_struct(struct fs_struct *fs);

extern struct fs_struct init_fs;

#endif /* _LINUX_FS_STRUCT_H */
