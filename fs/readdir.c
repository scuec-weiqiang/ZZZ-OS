#include <fs/types.h>
#include <fs/file.h>
#include <os/err.h>
#include <os/syscall_num.h>
#include <os/uaccess.h>

int iterate_dir(struct file *file, struct dir_context *ctx) {
	struct inode *inode = file_inode(file);
	int res = -ENOTDIR;
	if (!file->f_op->iterate)
		goto out;

	res = -ENOENT;
	if (S_ISDIR(inode->i_mode)) {
		ctx->pos = file->f_pos;
		res = file->f_op->iterate(file, ctx);
		file->f_pos = ctx->pos;
	}
	
out:
	return res;
}

/*
 * New, all-improved, singing, dancing, iBCS2-compliant getdents()
 * interface. 
 */
struct linux_dirent {
	u64		d_ino;
	s64		d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		d_name[0];
};

struct getdents_callback {
	struct dir_context ctx;
	struct linux_dirent __user * current_dir;
	struct linux_dirent __user * previous;
	int count;
	int error;
};

static int filldir(struct dir_context *ctx, const char *name, int namlen,
		     loff_t offset, u64 ino, unsigned int d_type)
{
	struct linux_dirent __user *dirent;
	struct getdents_callback *buf =
		container_of(ctx, struct getdents_callback, ctx);
	int reclen = ALIGN_UP(offsetof(struct linux_dirent, d_name) + namlen + 1, sizeof(u64));

	buf->error = -EINVAL;	/* only used if we fail.. */
	if (reclen > buf->count)
		return -EINVAL;
	dirent = buf->previous;
	if (dirent) {
		if (__put_user(offset, &dirent->d_off))
			goto efault;
	}
	dirent = buf->current_dir;
	if (__put_user(ino, &dirent->d_ino))
		goto efault;
	if (__put_user(0, &dirent->d_off))
		goto efault;
	if (__put_user(reclen, &dirent->d_reclen))
		goto efault;
	if (__put_user(d_type, &dirent->d_type))
		goto efault;
	if (copy_to_user(dirent->d_name, name, namlen))
		goto efault;
	if (__put_user(0, dirent->d_name + namlen))
		goto efault;
	buf->previous = dirent;
	dirent = (void __user *)dirent + reclen;
	buf->current_dir = dirent;
	buf->count -= reclen;
	return 0;
efault:
	buf->error = -EFAULT;
	return -EFAULT;
}

int getdents(unsigned int fd, struct linux_dirent __user * dirent, unsigned int count) {
	struct file *f;
	struct linux_dirent __user * lastdirent;
	struct getdents_callback buf = {
		.ctx.actor = filldir,
		.count = count,
		.current_dir = dirent
	};
	int error;

	f = fd_get_file(fd);
	if (!f)
		return -EBADF;

	error = iterate_dir(f, &buf.ctx);
	if (error >= 0)
		error = buf.error;
	lastdirent = buf.previous;
	if (lastdirent) {
		s64 d_off = buf.ctx.pos;
		if (__put_user(d_off, &lastdirent->d_off))
			error = -EFAULT;
		else
			error = count - buf.count;
	}
	fd_put_file(f);
	return error;
}


long sys_getdents(struct pt_regs *ctx) {
	unsigned int fd = ctx->r[0];
	struct linux_dirent __user *dirent = (struct linux_dirent __user *)ctx->r[1];
	unsigned int count = ctx->r[2];
	return getdents(fd, dirent, count);
}
