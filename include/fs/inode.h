#ifndef FS2_INODE_H
#define FS2_INODE_H

#include <fs/types.h>

int icache_init(void);
void icache_destroy(void);
struct inode *new_inode(struct super_block *sb);
struct inode *iget(struct super_block *sb, ino_t ino);
struct inode *igrab(struct inode *inode);
void iput(struct inode *inode);

#endif
