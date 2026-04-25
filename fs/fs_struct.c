#include <fs/types.h>
#include <fs/namei.h>
#include <os/kmalloc.h>
#include <os/sched.h>
#include <os/errno.h>

/*
 * Replace the fs->{rootmnt,root} with {mnt,dentry}. Put the old values.
 * It can block.
 */
void set_fs_root(struct fs_struct *fs, const struct path *path)
{
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
void set_fs_pwd(struct fs_struct *fs, const struct path *path)
{
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

void free_fs_struct(struct fs_struct *fs) {
	path_put(&fs->root);
	path_put(&fs->pwd);
	kfree(fs);
}

void exit_fs(struct task_struct *tsk) {
	struct fs_struct *fs = tsk->fs;

	if (fs) {
		int kill;
		spin_lock(&tsk->lock);
		spin_lock(&fs->lock);
		tsk->fs = NULL;
		kill = !--fs->users;
		spin_unlock(&fs->lock);
		spin_unlock(&tsk->lock);
		if (kill)
			free_fs_struct(fs);
	}
}

struct fs_struct *copy_fs_struct(struct fs_struct *old) {
	struct fs_struct *fs = kmalloc(sizeof(struct fs_struct));
	/* We don't need to lock fs - think why ;-) */
	if (fs) {
		fs->users = 1;
		fs->in_exec = 0;
		spin_lock_init(&fs->lock);
		fs->umask = old->umask;

		spin_lock(&old->lock);
		fs->root = old->root;
		path_get(&fs->root);
		fs->pwd = old->pwd;
		path_get(&fs->pwd);
		spin_unlock(&old->lock);
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
