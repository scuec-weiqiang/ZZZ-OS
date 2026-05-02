#include <fs/types.h>
#include <fs/namei.h>
#include <os/kmalloc.h>
#include <os/sched.h>
#include <os/errno.h>
#include <os/err.h>
#include <os/printk.h>
#include <mm/slab.h>

static struct kmem_cache *fs_struct_cache = NULL;

int alloc_fs_struct_init(void) {
	fs_struct_cache = kmem_cache_create("fs_struct_cache", sizeof(struct fs_struct), 8);
	if (!fs_struct_cache) {
		return -ENOMEM;
	}
	return 0;
}

struct fs_struct *alloc_fs_struct(void) {
	struct fs_struct *fs = kmem_cache_alloc(fs_struct_cache);
	if (!fs) {
		return ERR_PTR(-ENOMEM);
	}
	fs->users = 1;
	return fs;
}

void free_fs_struct(struct fs_struct *fs) {
	kmem_cache_free(fs);
}

/*
 * Replace the fs->{rootmnt,root} with {mnt,dentry}. Put the old values.
 * It can block.
 */
void set_fs_root(struct fs_struct *fs, const struct path *path) {
	struct path old_root;

	path_get(path);
	spin_lock(&fs->lock);

	old_root = fs->root;
	fs->root = *path;

	spin_unlock(&fs->lock);
	if (old_root.dentry)
		path_put(&old_root);
}

/*
 * Replace the fs->{pwdmnt,pwd} with {mnt,dentry}. Put the old values.
 * It can block.
 */
void set_fs_pwd(struct fs_struct *fs, const struct path *path) {
	struct path old_pwd;

	path_get(path);
	spin_lock(&fs->lock);
	old_pwd = fs->pwd;
	fs->pwd = *path;
	spin_unlock(&fs->lock);

	if (old_pwd.dentry)
		path_put(&old_pwd);
}

// static inline int replace_path(struct path *p, const struct path *old, const struct path *new)
// {
// 	if (likely(p->dentry != old->dentry || p->mnt != old->mnt))
// 		return 0;
// 	*p = *new;
// 	return 1;
// }

struct fs_struct *get_fs_struct(struct fs_struct *fs) {
	if (fs) {
		spin_lock(&fs->lock);
		fs->users ++;
		spin_unlock(&fs->lock);
	}
	return fs;
}

void put_fs_struct(struct fs_struct *fs) {
	if (fs) {
		spin_lock(&fs->lock);
		if (--fs->users == 0) {
			spin_unlock(&fs->lock);
			free_fs_struct(fs);
			return;
		}
		spin_unlock(&fs->lock);
	}
}



struct fs_struct *copy_fs_struct(struct fs_struct *old) {
	struct fs_struct *fs = alloc_fs_struct();

	if (fs) {
		fs->users = 1;
		spin_lock_init(&fs->lock);

		fs->umask = old->umask;
		
		fs->root = old->root;
		path_get(&fs->root);

		fs->pwd = old->pwd;
		path_get(&fs->pwd);
	}
	return fs;
}

int unshare_fs_struct(void) {
	struct fs_struct *fs = current->fs;
	struct fs_struct *new_fs = copy_fs_struct(fs);
	int kill;

	if (!new_fs)
		return -ENOMEM;

	spin_lock(&current->lock);
	spin_lock(&fs->lock);
	kill = !--fs->users;
	current->fs = new_fs;
	spin_unlock(&fs->lock);
	spin_unlock(&current->lock);

	if (kill)
		free_fs_struct(fs);

	return 0;
}

int current_umask(void) {
	return current->fs->umask;
}

/* to be mentioned only in INIT_TASK */
struct fs_struct init_fs = {
	.users		= 1,
	.lock		= SPINLOCK_INIT,
	.umask		= 0022,
};
