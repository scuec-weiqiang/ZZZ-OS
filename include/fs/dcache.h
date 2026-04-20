#ifndef FS2_DCACHE_H
#define FS2_DCACHE_H

#include <fs/types.h>

int dcache_init(void);
void dcache_destroy(void);
struct dentry *d_lookup(struct dentry *parent, const struct qstr *name);
struct dentry *d_alloc(struct dentry *parent, const char *name);
struct dentry *d_alloc_qstr(struct dentry *parent, const struct qstr *name);
void d_add(struct dentry *dentry, struct inode *inode);
void d_destroy(struct dentry *dentry);
struct dentry *d_make_root(struct inode *root_inode);
struct dentry *dget(struct dentry *dentry);
void dput(struct dentry *dentry);

#endif
