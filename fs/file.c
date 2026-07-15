#include <fs/file.h>
#include <fs/blkdev.h>
#include <fs/cdev.h>
#include <fs/dcache.h>
#include <fs/inode.h>
#include <fs/namei.h>
#include <fs/fs_struct.h>
#include <os/check.h>
#include <os/err.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/errno.h>
#include <os/atomic.h>
#include <os/spinlock.h>
#include <os/sched.h>
#include <asm/syscall_num.h>
#include <os/uaccess.h>
#include <os/minmax.h>
#include <mm/slab.h>
#include <os/syscall.h>

struct kmem_cache *file_kmem_cache = NULL;

int alloc_file_init(void) {
    file_kmem_cache = kmem_cache_create("file_cache", sizeof(struct file), 0);
    if (!file_kmem_cache) {
        return -ENOMEM;
    }
    return 0;
}

struct file *alloc_file(void) {
    return kmem_cache_alloc(file_kmem_cache);
}

void free_file(struct file *file) {
    if (file) {
        kmem_cache_free(file);
    }
}

#define SYSCALL_PATH_MAX 256
#define SYSCALL_IO_CHUNK 512

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4
#define F_DUPFD_CLOEXEC 1030
#define FD_CLOEXEC 1
#define FCNTL_SETTABLE_FLAGS (O_APPEND | O_NONBLOCK)

static struct file *sys_fdget(int fd) {
    struct files_struct *files;
    struct file *file = NULL;

    if (fd < 0)
        return NULL;

    files = current->files;
    if (files == NULL)
        return NULL;

    spin_lock(&files->file_lock);
    if (fd < files->fdtab.max_fds && fd_is_open(fd, &files->fdtab))
        file = files->fdtab.fd[fd];
    spin_unlock(&files->file_lock);

    return file;
}

static int fd_get_close_on_exec(int fd)
{
    struct files_struct *files;
    int ret = -EBADF;

    if (fd < 0)
        return -EBADF;

    files = current->files;
    if (files == NULL)
        return -EBADF;

    spin_lock(&files->file_lock);
    if (fd < files->fdtab.max_fds && fd_is_open(fd, &files->fdtab))
        ret = close_on_exec(fd, &files->fdtab) ? FD_CLOEXEC : 0;
    spin_unlock(&files->file_lock);

    return ret;
}

__SYSCALL__ long sys_fcntl(struct pt_regs *ctx)
{
    int fd = (int)ctx->r[0];
    int cmd = (int)ctx->r[1];
    unsigned long arg = ctx->r[2];
    struct file *file;
    int newfd;

    switch (cmd) {
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
        if ((int)arg < 0)
            return -EINVAL;

        file = fd_get_file(fd);
        if (file == NULL)
            return -EBADF;

        newfd = alloc_fd((unsigned)arg,
                         cmd == F_DUPFD_CLOEXEC ? O_CLOEXEC : 0);
        if (newfd < 0) {
            fd_put_file(file);
            return newfd;
        }

        attach_fd((unsigned)newfd, file);
        return newfd;

    case F_GETFD:
        return fd_get_close_on_exec(fd);

    case F_SETFD:
        if (sys_fdget(fd) == NULL)
            return -EBADF;
        set_close_on_exec((unsigned)fd, arg & FD_CLOEXEC);
        return 0;

    case F_GETFL:
        file = sys_fdget(fd);
        if (file == NULL)
            return -EBADF;
        return file->f_flags;

    case F_SETFL:
        file = sys_fdget(fd);
        if (file == NULL)
            return -EBADF;
        file->f_flags = (file->f_flags & ~FCNTL_SETTABLE_FLAGS) |
                        (arg & FCNTL_SETTABLE_FLAGS);
        return 0;

    default:
        return -EINVAL;
    }
}

# define	SEEK_SET	0
# define	SEEK_CUR	1
# define	SEEK_END	2

#ifndef F_OK
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

off_t generic_file_lseek(struct file *file, off_t offset, int whence) {
    off_t newpos;

    switch (whence) {
    case SEEK_SET:
        newpos = offset;
        break;
    case SEEK_CUR:
        newpos = file->f_pos + offset;
        break;
    case SEEK_END:
        newpos = file->f_inode->i_size + offset;
        break;
    default:
        return -EINVAL;
    }

    if (newpos < 0)
        return -EINVAL;

    file->f_pos = newpos;
    return newpos;
}

__SYSCALL__ long sys_read(struct pt_regs *ctx) {
    int fd = (int)ctx->r[0];
    char *user_buf = (char *)ctx->r[1];
    size_t len = ctx->r[2];
    struct file *file;
    char *kbuf;
    size_t done = 0;
    ssize_t ret;

    if (len == 0)
        return 0;

    file = sys_fdget(fd);
    if (file == NULL)
        return -EBADF;
    if (file->f_flags & O_WRONLY)
        return -EACCES;

    kbuf = kmalloc(SYSCALL_IO_CHUNK);
    if (kbuf == NULL)
        return -ENOMEM;

    while (done < len) {
        size_t chunk = min(len - done, (size_t)SYSCALL_IO_CHUNK);

        ret = kernel_read(file, kbuf, chunk);
        if (ret < 0) {
            if (done == 0) {
                kfree(kbuf);
                return ret;
            }
            break;
        }
        if (ret == 0)
            break;

        if (copy_to_user(user_buf + done, kbuf, (size_t)ret) != 0) {
            kfree(kbuf);
            return done ? (ssize_t)done : -EFAULT;
        }

        done += (size_t)ret;
        if ((size_t)ret < chunk)
            break;
    }
    // dprintk("sys_read fd=%d read_len=%lu\n", fd, done);
    kfree(kbuf);
    return (ssize_t)done;
}

__SYSCALL__ long sys_write(struct pt_regs *ctx) {
    int fd = (int)ctx->r[0];
    const char *user_buf = (const char *)ctx->r[1];
    size_t len = ctx->r[2];
    struct file *file;
    char *kbuf;
    size_t done = 0;
    ssize_t ret;

    if (len == 0)
        return 0;

    file = sys_fdget(fd);
    if (file == NULL)
        return -EBADF;

    if (file->f_flags & O_RDONLY)
        return -EACCES;

    kbuf = kmalloc(SYSCALL_IO_CHUNK);
    if (kbuf == NULL)
        return -ENOMEM;

    while (done < len) {
        size_t chunk = min(len - done, (size_t)SYSCALL_IO_CHUNK);

        if (copy_from_user(kbuf, user_buf + done, chunk) < 0) {
            kfree(kbuf);
            return done ? (ssize_t)done : -EFAULT;
        }

        ret = kernel_write(file, kbuf, chunk);
        if (ret < 0) {
            if (done == 0) {
                kfree(kbuf);
                return ret;
            }
            break;
        }
        if (ret == 0)
            break;

        done += (size_t)ret;
        if ((size_t)ret < chunk)
            break;
    }

    kfree(kbuf);
    return (ssize_t)done;
}

__SYSCALL__ long sys_open(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    int flags = (int)ctx->r[1];
    char path[SYSCALL_PATH_MAX];
    struct file *file;
    int fd;

    memset(path, 0, SYSCALL_PATH_MAX);
    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    file = filp_open(path, (u32)flags);
    if (IS_ERR(file)) {
        long err = PTR_ERR(file);

        if ((flags & O_CREAT) == 0 || err != -ENOENT)
            return err;

        if (vfs_create(path, 0644) == NULL) {
            return -ENOENT;
        }

        file = filp_open(path, (u32)flags);
        if (IS_ERR(file))
            return PTR_ERR(file);
    }

    fd = alloc_fd(0, (unsigned)flags);
    if (fd < 0) {
        filp_close(file);
        return fd;
    }

    attach_fd(fd, file);
    return fd;
}

__SYSCALL__ long sys_close(struct pt_regs *ctx) {
    return close_fd((unsigned)ctx->r[0]);
}

__SYSCALL__ long sys_creat(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    int mode = (int)ctx->r[1];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    if (vfs_create(path, (u16)mode) == NULL)
        return -EIO;

    return sys_open(ctx);
}

__SYSCALL__ long sys_mkdir(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    int mode = (int)ctx->r[1];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    mode &= ~current_umask();

    return vfs_mkdir(path, (u16)mode);
}

__SYSCALL__ long sys_umask(struct pt_regs *ctx) {
    int mask = (int)ctx->r[0] & 0777;
    int old;

    if (current->fs == NULL)
        return -EINVAL;

    spin_lock(&current->fs->lock);
    old = current->fs->umask;
    current->fs->umask = mask;
    spin_unlock(&current->fs->lock);

    return old;
}

__SYSCALL__ long sys_access(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    int mode = (int)ctx->r[1];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    if (mode != F_OK && (mode & ~(R_OK | W_OK | X_OK)))
        return -EINVAL;

    return vfs_access(path, mode);
}

__SYSCALL__ long sys_rmdir(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    return vfs_rmdir(path);
}

__SYSCALL__ long sys_unlink(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    return vfs_unlink(path);
}

__SYSCALL__ long sys_chdir(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    return vfs_chdir(path);
}

__SYSCALL__ long sys_getcwd(struct pt_regs *ctx) {
    uintptr_t user_buf = ctx->r[0];
    size_t size = (size_t)ctx->r[1];
    char *kbuf;
    int len;

    if (user_buf == 0 || size == 0) {
        return -EINVAL;
    }

    kbuf = kmalloc(size);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    len = vfs_getcwd(kbuf, size);
    if (len < 0) {
        kfree(kbuf);
        return len;
    }

    if (copy_to_user((char *)user_buf, kbuf, (size_t)len) != 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    kfree(kbuf);
    return len;
}

__SYSCALL__ long sys_lseek(struct pt_regs *ctx) {
    int fd = (int)ctx->r[0];
    off_t offset = (off_t)ctx->r[1];
    int whence = (int)ctx->r[2];
    struct file *file;
    off_t new_pos;

    file = sys_fdget(fd);
    if (file == NULL)
        return -EBADF;

    if (file->f_op == NULL || file->f_op->lseek == NULL)
        return -EINVAL;

    new_pos = file->f_op->lseek(file, offset, whence);
    if (new_pos < 0)
        return new_pos;

    return new_pos;
}

__SYSCALL__ long sys_mkdirat(struct pt_regs *ctx) {
    struct pt_regs mkdir_ctx = *ctx;

    mkdir_ctx.r[0] = ctx->r[1];
    mkdir_ctx.r[1] = ctx->r[2];
    return sys_mkdir(&mkdir_ctx);
}

__SYSCALL__ long sys_unlinkat(struct pt_regs *ctx) {
    struct pt_regs unlink_ctx = *ctx;

    unlink_ctx.r[0] = ctx->r[1];
    if (ctx->r[2] & 0x200)
        return sys_rmdir(&unlink_ctx);

    return sys_unlink(&unlink_ctx);
}



__SYSCALL__ long sys_openat(struct pt_regs *ctx) {
    struct pt_regs open_ctx = *ctx;

    open_ctx.r[0] = ctx->r[1];
    open_ctx.r[1] = ctx->r[2];
    return sys_open(&open_ctx);
}


struct file *filp_open(const char *path, u32 flags) {
    struct cdev *cdev = NULL;
    struct blkdev *bdev = NULL;
    struct file *file = NULL;
    struct path resolved = {0};
    int ret = 0;

    if (flags & O_NOFOLLOW)
        ret = path_lookup_nofollow(path, &resolved);
    else
        ret = path_lookup(path, &resolved);
    if (ret < 0) {
         return ERR_PTR(ret);
        
    }

    if (resolved.dentry == NULL || resolved.dentry->d_inode == NULL) {
        ret = -ENOENT;
        goto err_put_path;
    }
    if ((flags & O_NOFOLLOW) && S_ISLNK(resolved.dentry->d_inode->i_mode)) {
        ret = -ELOOP;
        goto err_put_path;
    }
    
    file = alloc_file();
    if (file == NULL) {
        ret = -ENOMEM;
        goto err_put_path;
    }
    memset(file, 0, sizeof(*file));

    file->f_path = resolved;
    file->f_inode = resolved.dentry->d_inode;
    file->f_op = file->f_inode->i_fop;
    file->f_flags = flags;
    atomic_set(&file->f_count, 1);

    if (S_ISCHR(file->f_inode->i_mode)) {
        cdev = cdev_get_by_devnr(file->f_inode->i_rdev);
		
        if (cdev == NULL) {
            ret = -ENODEV;
            goto err_free_file;
        }
        cdev->cd_openers++;
        file->f_op = (struct file_operations *)cdev->fops;
		// dprintk("filp_open: open char device %s, devnr=%u, fops=%xu\n",
		// 		cdev->name, cdev->devnr, file->f_op);
        file->private_data = cdev->private ? cdev->private : cdev;
    } else if (S_ISBLK(file->f_inode->i_mode)) {
        bdev = blkdev_get_by_devnr(file->f_inode->i_rdev);
        if (bdev == NULL) {
            ret = -ENODEV;
            goto err_free_file;
        }
        bdev->bd_openers++;
        file->f_op = bdev->bd_fops;
        file->private_data = bdev;
    }

    if (file->f_op != NULL && file->f_op->open != NULL) {
        ret = file->f_op->open(file->f_inode, file);
        if (ret != 0) {
            if (ret > 0) {
                ret = -EIO;
            }
            goto err_free_file;
        }
    }

    return file;

err_free_file:
    if (cdev != NULL) {
        cdev_put(cdev);
    }
    if (bdev != NULL) {
        blkdev_put(bdev);
    }
    free_file(file);
err_put_path:
    file = NULL;
    if (resolved.dentry != NULL || resolved.mnt != NULL) {
        path_put(&resolved);
    }
    return ERR_PTR(ret);
}

void filp_close(struct file *file) {
    if (file != NULL) {
        if (file->f_inode != NULL) {
            if (S_ISCHR(file->f_inode->i_mode)) {
                struct cdev *cdev = cdev_get_by_devnr(file->f_inode->i_rdev);
                cdev_put(cdev);
            } else if (S_ISBLK(file->f_inode->i_mode)) {
                struct blkdev *bdev = blkdev_get_by_devnr(file->f_inode->i_rdev);
                blkdev_put(bdev);
            }
        }
        if (file->f_path.dentry != NULL || file->f_path.mnt != NULL) {
            path_put(&file->f_path);
        }
        free_file(file);
    }
}

ssize_t kernel_read(struct file *file, char *buf, size_t len) {
    RETURN_ERR_IF(file == NULL, -EBADF);
    RETURN_ERR_IF(file->f_op == NULL || file->f_op->read == NULL, -EINVAL);
    return file->f_op->read(file, buf, len, &file->f_pos);
}

ssize_t kernel_read_at(struct file *file, loff_t pos, char *buf, size_t len) {
    loff_t saved_pos;
    ssize_t ret;

    RETURN_ERR_IF(file == NULL, -EBADF);
    RETURN_ERR_IF(file->f_op == NULL || file->f_op->read == NULL, -EINVAL);

    saved_pos = file->f_pos;
    file->f_pos = pos;
    ret = file->f_op->read(file, buf, len, &file->f_pos);
    file->f_pos = saved_pos;

    return ret;
}

ssize_t kernel_write(struct file *file, const char *buf, size_t len) {
	
    RETURN_ERR_IF(file == NULL, -EBADF);
	
    RETURN_ERR_IF(file->f_op == NULL || file->f_op->write == NULL, -EINVAL);
	
    return file->f_op->write(file, buf, len, &file->f_pos);
}

// close_on_exec 和 open_fds 都是位图，fd对应的位为1表示该fd被设置了close_on_exec或者是open的
static inline void __set_close_on_exec(int fd, struct fdtable *fdt) {
	__set_bit(fd, fdt->close_on_exec);
}

static inline void __clear_close_on_exec(int fd, struct fdtable *fdt) {
	__clear_bit(fd, fdt->close_on_exec);
}

static inline void __set_open_fd(int fd, struct fdtable *fdt) {
	__set_bit(fd, fdt->open_fds);
}

static inline void __clear_open_fd(int fd, struct fdtable *fdt) {
	__clear_bit(fd, fdt->open_fds);
}

static void init_files_struct(struct files_struct *files) {
    memset(files, 0, sizeof(*files));
    atomic_set(&files->refcount, 1);
    spin_lock_init(&files->file_lock);
    files->fdtab.max_fds = MAX_OPEN_FILES_NUM;
    files->fdtab.close_on_exec = files->close_on_exec_init;
    files->fdtab.open_fds = files->open_fds_init;
    files->fdtab.fd = &files->fd_array[0];
    files->next_fd = 0;
}

#include <mm/slab.h>

static struct kmem_cache *files_struct_cache = NULL;

int alloc_files_struct_init(void) {
    files_struct_cache = 
    kmem_cache_create("files_struct_cache", sizeof(struct files_struct), 8);
    if (!files_struct_cache) {
        return -ENOMEM;
    }
    return 0;
}

struct files_struct *alloc_files_struct(void) {
    struct files_struct *files = kmem_cache_alloc(files_struct_cache);
    if (!files) {
        return ERR_PTR(-ENOMEM);
    }
    init_files_struct(files);
    return files;
}

void free_files_struct(struct files_struct *files) {
    if (!files)
        return;
    kmem_cache_free(files);
}

static void file_release_last_ref(struct file *file) {
    if (file && atomic_read(&file->f_count) == 0) {
        if (file->f_op && file->f_op->release) {
            file->f_op->release(file->f_inode, file);
        }
        filp_close(file);
    }
}

struct files_struct *dup_fd(struct files_struct *oldf, unsigned long clone_flags) {
	struct files_struct *newf;
	int i;

	if (oldf == NULL) {
		return ERR_PTR(-EINVAL);
	}
	if (clone_flags & CLONE_FILES) {
		atomic_inc(&oldf->refcount);
		return oldf;
	}

	newf = alloc_files_struct();
	if (IS_ERR(newf)) {
        goto out;
    }

	spin_lock(&oldf->file_lock);
	for (i = 0; i < MAX_OPEN_FILES_NUM; i++) {
        struct file *f;

        // 只复制被打开的fd
        if (!fd_is_open(i, &oldf->fdtab))
            continue;

        f = oldf->fdtab.fd[i];
        if (!f)
            continue;

        newf->fdtab.fd[i] = get_file(f);
        __set_open_fd(i, &newf->fdtab);
        if (close_on_exec(i, &oldf->fdtab))
            __set_close_on_exec(i, &newf->fdtab);
	}
	spin_unlock(&oldf->file_lock);

out:
	return newf;
}

static struct fdtable *close_files(struct files_struct * files) {
	/*
	 * It is safe to dereference the fd table without RCU or
	 * ->file_lock because this is the last reference to the
	 * files structure.
	 */ 
	struct fdtable *fdt = &files->fdtab;
	int i, j = 0;

	for (;;) {
		unsigned long set;
		i = j * BITS_PER_LONG;
		if (i >= fdt->max_fds)
			break;
		set = fdt->open_fds[j++];
		while (set) {
				if (set & 1) {
					struct file * file = fdt->fd[i];
					if (file) {
	                    fdt->fd[i] = NULL;
						put_file(file);
	                    file_release_last_ref(file);
					}
				}
			i++;
			set >>= 1;
		}
	}

	return fdt;
}

struct files_struct init_files = {
	.refcount		= ATOMIC_INIT(1),
	.fdtab		= {
		.max_fds	= MAX_OPEN_FILES_NUM,
		.fd		= &init_files.fd_array[0],
		.close_on_exec	= init_files.close_on_exec_init,
		.open_fds	= init_files.open_fds_init,
	},
	.file_lock	= SPINLOCK_INIT,
};

#include <os/bitops.h>

// 从 addr 指向的位图中，从 offset 开始找下一个 0 bit，返回其位置
unsigned long find_next_zero_bit(const unsigned long *addr, unsigned long size, unsigned long offset) {
    // 当前处理到第几个 unsigned long
    unsigned long k = offset / (8 * sizeof(unsigned long));
    // 在当前 unsigned long 内部的起始 bit 偏移
    unsigned long idx = offset % (8 * sizeof(unsigned long));

    unsigned long mask;

    // 从 offset 开始，先处理当前这个 word 里剩下的 bit
    if (idx) {
        // 只保留从 idx 开始到最后的 bit，前面全部抹成 1（不影响找 0）
        mask = ~((1UL << idx) - 1);
        if ((addr[k] | mask) != ~0UL) {
            // 这个 word 里还有 0，直接找
            unsigned long bit = __ffs(~(addr[k] | mask));
            unsigned long pos = k * BITS_PER_LONG + bit;
            return pos < size ? pos : size;
        }
        // 当前 word 已满，跳到下一个 word
        k++;
    }

    // 逐整个 word 扫描，找不是全 1 的 word
    for (; k * BITS_PER_LONG < size; k++) {
        if (addr[k] != ~0UL) {
            // 找到有 0 的 word，找第一个 0
            unsigned long bit = __ffs(~addr[k]);
            unsigned long pos = k * BITS_PER_LONG + bit;
            return pos < size ? pos : size;
        }
    }

    // 没找到，返回 size（表示满了）
    return size;
}

static void __put_unused_fd(struct files_struct *files, unsigned int fd) {
	struct fdtable *fdt = &files->fdtab;
	__clear_open_fd(fd, fdt);
	if (fd < files->next_fd)
		files->next_fd = fd;
}

static int __alloc_fd(struct files_struct *files, unsigned start, unsigned end, unsigned flags) {
	unsigned int fd;

	spin_lock(&files->file_lock);
	fd = start > (unsigned)files->next_fd ? start : (unsigned)files->next_fd;
    while (fd < end) {
        if (!fd_is_open((int)fd, &files->fdtab))
            break;
        fd++;
    }

    if (fd >= end) {
        spin_unlock(&files->file_lock);
        return -EMFILE;
    }

	__set_open_fd(fd, &files->fdtab);
	if (flags & O_CLOEXEC)
		__set_close_on_exec(fd, &files->fdtab);
	else
		__clear_close_on_exec(fd, &files->fdtab);
    files->fdtab.fd[fd] = NULL;
    files->next_fd = fd + 1;
	spin_unlock(&files->file_lock);
	return (int)fd;
}

static int __close_fd(struct files_struct *files, unsigned fd) {
	if (files == NULL || fd >= MAX_OPEN_FILES_NUM) {
		return -EBADF;
	}

	struct file *file;
	struct fdtable *fdt;

	spin_lock(&files->file_lock);

	fdt = &files->fdtab;
	if (fd >= fdt->max_fds)
		goto out_unlock;

	file = fdt->fd[fd];
	if (!file)
		goto out_unlock;

	fdt->fd[fd] = NULL;
	__clear_close_on_exec(fd, fdt);
	__put_unused_fd(files, fd);
    
	spin_unlock(&files->file_lock);

	put_file(file);
    file_release_last_ref(file);
		
    return 0;

out_unlock:
	spin_unlock(&files->file_lock);
	return -EBADF;
}

int alloc_fd(unsigned start, unsigned flags) {
	return __alloc_fd(current->files, start, MAX_OPEN_FILES_NUM, flags);
}

int close_fd(unsigned fd) {
	return __close_fd(current->files, (unsigned)fd);
}

// 将文件描述符和文件绑定
void attach_fd(unsigned int fd, struct file *file) {
    struct files_struct *files = current->files;

    spin_lock(&files->file_lock);

    __set_open_fd((int)fd, &files->fdtab);
    files->fdtab.fd[fd] = file;

    spin_unlock(&files->file_lock);
}
static bool stdio_status = false;

int setup_stdio(const char *path) {
    static const unsigned stdio_fds[3] = {0, 1, 2};
    struct files_struct *files = current->files;
    unsigned i;

    if (!path || !files)
        return -EINVAL;

    for (i = 0; i < 3; i++) {
        struct file *file = filp_open(path, O_RDWR);

        if (IS_ERR(file)) {
            return PTR_ERR(file);
        }
            

        if (close_fd(stdio_fds[i]) < 0) {
            /* Ignore EBADF for empty stdio slots in a fresh files table. */
        }

        spin_lock(&files->file_lock);

        files->fdtab.fd[stdio_fds[i]] = file;
        __set_open_fd((int)stdio_fds[i], &files->fdtab);
        __clear_close_on_exec((int)stdio_fds[i], &files->fdtab);
        if (files->next_fd <= (int)stdio_fds[i])
            files->next_fd = (int)stdio_fds[i] + 1;

        spin_unlock(&files->file_lock);
    }

    stdio_status = true;

    return 0;
}

bool stdio_is_setup(void) {
    return stdio_status;
}

struct file *fd_get_file(unsigned int fd) {
	struct files_struct *files = current->files;
	struct file *file = NULL;

	spin_lock(&files->file_lock);
	if (fd < files->fdtab.max_fds && test_bit(fd, files->fdtab.open_fds)) {
		file = files->fdtab.fd[fd];
		if (file)
			get_file(file);
	}
	spin_unlock(&files->file_lock);

	return file;
}

void fd_put_file(struct file *file) {
	if (file)
		put_file(file);
}

void do_close_on_exec(struct files_struct *files) {
	unsigned fd;

	for (fd = 0; fd < MAX_OPEN_FILES_NUM; fd++) {
		struct file *file;

		spin_lock(&files->file_lock);

		if (!fd_is_open((int)fd, &files->fdtab) ||
		    !close_on_exec((int)fd, &files->fdtab)) {
			spin_unlock(&files->file_lock);
			continue;
		}

		file = files->fdtab.fd[fd];
		files->fdtab.fd[fd] = NULL;
		__clear_close_on_exec((int)fd, &files->fdtab);
		__put_unused_fd(files, fd);

		spin_unlock(&files->file_lock);

		put_file(file);
		file_release_last_ref(file);
	}
}

void set_close_on_exec(unsigned int fd, int flag) {
	struct files_struct *files = current->files;
	struct fdtable *fdt;
	spin_lock(&files->file_lock);
	fdt = &files->fdtab;
	if (flag)
		__set_close_on_exec(fd, fdt);
	else
		__clear_close_on_exec(fd, fdt);
	spin_unlock(&files->file_lock);
}

struct files_struct *get_files_struct(struct task_struct *task) {
	struct files_struct *files;

	spin_lock(&task->lock);
	files = task->files;
	if (files)
		atomic_inc(&files->refcount);
	spin_unlock(&task->lock);

	return files;
}

void put_files_struct(struct files_struct *files) {
    if (!files || files == &init_files)
        return;

    if (atomic_read(&files->refcount) > 1) {
        atomic_dec(&files->refcount);
        return;
    }

    close_files(files);
    kmem_cache_free(files);
}

int unshare_files_struct(void)
{
    struct files_struct *old_files;
    struct files_struct *new_files;

    old_files = current->files;
    if (old_files == NULL) {
        return -EINVAL;
    }

    if (atomic_read(&old_files->refcount) <= 1) {
        return 0;
    }

    new_files = dup_fd(old_files, 0);
    if (IS_ERR(new_files)) {
        return PTR_ERR(new_files);
    }

    spin_lock(&current->lock);
    current->files = new_files;
    spin_unlock(&current->lock);

    put_files_struct(old_files);
    return 0;
}

__SYSCALL__ long sys_fstat(struct pt_regs *ctx) {
    int fd = (int)ctx->r[0];
    void* user_st = (void*)ctx->r[1];
    struct user_stat st;
    struct file *file;
    struct inode *inode;

    file = sys_fdget(fd);
    if (file == NULL)
        return -EBADF;

    memset(&st, 0, sizeof(st));

    inode = file->f_inode;
    if (inode) {
        st.st_ino = (ino_t)inode->i_ino;         // inode 号
        st.st_size = (off_t)inode->i_size;          // 文件大小
        st.st_atim.tv_sec = inode->i_atime.tv_sec;        // 访问时间
        st.st_mtim.tv_sec = inode->i_mtime.tv_sec;        // 修改时间
        st.st_ctim.tv_sec = inode->i_ctime.tv_sec;        // 创建时间
        st.st_blksize = 512;
        st.st_blocks = (inode->i_size + 511) / 512;

       
        if (S_ISREG(inode->i_mode)) {
            st.st_mode = S_IFREG | 0644;   // 普通文件
        } else if (S_ISDIR(inode->i_mode)) {
            st.st_mode = S_IFDIR | 0755;   // 目录
        } else if (S_ISCHR(inode->i_mode)) {
            st.st_mode = S_IFCHR | 0666;   // 控制台/串口
        }
    
        st.st_nlink = inode->i_nlink;      // 链接数
        st.st_uid = 0;
        st.st_gid = 0;
    } else {
        return -EBADF;
    }

    if (copy_to_user( user_st, (char *)&st,sizeof(st))) {
        return -EFAULT;
    }

    return 0; // 成功
}

__SYSCALL__ long sys_stat(struct pt_regs *ctx) {
    uintptr_t user_path = ctx->r[0];
    void* user_st = (void*)ctx->r[1];
    char path[SYSCALL_PATH_MAX];
    struct user_stat st;
    struct path resolved = {0};
    struct inode *inode;
    long ret = 0;

    if (copy_user_string(path, sizeof(path), user_path) < 0)
        return -EFAULT;

    if (path_lookup(path, &resolved) < 0)
        return -ENOENT;

    memset(&st, 0, sizeof(st));

    inode = resolved.dentry->d_inode;
    if (inode) {
        st.st_ino = (ino_t)inode->i_ino;         // inode 号
        st.st_size = (off_t)inode->i_size;          // 文件大小
        st.st_atim.tv_sec = inode->i_atime.tv_sec;        // 访问时间
        st.st_mtim.tv_sec = inode->i_mtime.tv_sec;        // 修改时间
        st.st_ctim.tv_sec = inode->i_ctime.tv_sec;        // 创建时间
        st.st_blksize = 512;
        st.st_blocks = (inode->i_size + 511) / 512;

       
        if (S_ISREG(inode->i_mode)) {
            st.st_mode = S_IFREG | 0644;   // 普通文件
        } else if (S_ISDIR(inode->i_mode)) {
            st.st_mode = S_IFDIR | 0755;   // 目录
        } else if (S_ISCHR(inode->i_mode)) {
            st.st_mode = S_IFCHR | 0666;   // 控制台/串口
        }
    
        st.st_nlink = inode->i_nlink;      // 链接数
        st.st_uid = 0;
        st.st_gid = 0;
    } else {
        ret = -ENOENT;
        goto out_put_path;
    }

    if (copy_to_user( user_st, (char *)&st,sizeof(st))) {
        ret = -EFAULT;
        goto out_put_path;
    }

out_put_path:
    path_put(&resolved);
    return ret; // 成功
}

__SYSCALL__ long sys_newfstatat(struct pt_regs *ctx) {
    struct pt_regs stat_ctx = *ctx;

    stat_ctx.r[0] = ctx->r[1];
    stat_ctx.r[1] = ctx->r[2];
    return sys_stat(&stat_ctx);
}

__SYSCALL__ long sys_dup(struct pt_regs *ctx) {
    int oldfd = ctx->r[0];
    struct file *file;
    int newfd;

    file = fd_get_file(oldfd);
    if (!file)
        return -EBADF;

    newfd = alloc_fd(0, 0);
    if (newfd < 0) {
        fd_put_file(file);
        return newfd;
    }

    attach_fd(newfd, file);

    return newfd;
}

__SYSCALL__ long sys_dup2(struct pt_regs *ctx) {   
    int oldfd = ctx->r[0];
    int newfd = ctx->r[1];
    struct file *file;

    if (oldfd == newfd)
        return newfd;

    file = fd_get_file(oldfd);
    if (!file)
        return -EBADF;

    __close_fd(current->files, newfd);

    attach_fd(newfd, file);

    return newfd;
}

__SYSCALL__ long sys_dup3(struct pt_regs *ctx) {
    if (ctx->r[2] != 0)
        return -EINVAL;

    return sys_dup2(ctx);
}

__SYSCALL__ long sys_faccessat(struct pt_regs *ctx) {
    struct pt_regs access_ctx = *ctx;

    access_ctx.r[0] = ctx->r[1];
    access_ctx.r[1] = ctx->r[2];
    return sys_access(&access_ctx);
}

__SYSCALL__ long sys_faccessat2(struct pt_regs *ctx) {
    unsigned int flags = (unsigned int)ctx->r[3];

    if (flags & ~(AT_SYMLINK_NOFOLLOW | AT_EACCESS | AT_EMPTY_PATH))
        return -EINVAL;

    return sys_faccessat(ctx);
}

__SYSCALL__ long sys_readlinkat(struct pt_regs *ctx) {
    const char *pathname = (const char *)ctx->r[1];
    char *user_buf = (char *)ctx->r[2];
    size_t bufsiz = ctx->r[3];
    char path_buf[256];
    char *kbuf;
    ssize_t ret;

    if (bufsiz == 0)
        return -EINVAL;

    if (copy_user_string(path_buf, sizeof(path_buf), (uintptr_t)pathname) < 0)
        return -EFAULT;

    kbuf = kmalloc(bufsiz);
    if (kbuf == NULL)
        return -ENOMEM;

    ret = vfs_readlink(path_buf, kbuf, bufsiz);
    if (ret < 0)
        goto out;

    if (copy_to_user(user_buf, kbuf, ret) < 0) {
        ret = -EFAULT;
        goto out;
    }

out:
    kfree(kbuf);
    return ret;
}

SYSCALL_DEFINE3(ioctl, int, fd, unsigned long, request, unsigned long, arg) {
    struct file *file;
    struct inode *inode;
    long ret;

    file = fd_get_file((unsigned int)fd);
    if (file == NULL) {
        return -EBADF;
    }

    inode = file->f_inode;
    if (inode == NULL || !S_ISCHR(inode->i_mode)) {
        ret = -ENOTTY;
        goto out_put;
    }

    if (file->f_op == NULL || file->f_op->ioctl == NULL) {
        ret = -ENOTTY;
    } else {
        ret = file->f_op->ioctl(file, request, arg);
    }

out_put:
    fd_put_file(file);
#ifdef SYS_TRACE_ENABLE
    if (ret == -ENOTTY) {
        printk("[ioctl] fd=%d request=%lx arg=%lx ret=%ld\n", fd, request, arg, ret);
    }
#endif
    return ret;
}
