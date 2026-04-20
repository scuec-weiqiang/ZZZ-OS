#include <fs/file.h>
#include <fs/dcache.h>
#include <fs/inode.h>
#include <fs/namei.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/errno-base.h>
#include <os/atomic.h>
#include <os/spinlock.h>
#include <os/sched.h>

struct file *filp_open(const char *path, uint32_t flags) {
    struct file *file = NULL;
    struct path resolved = {0};
    int ret = 0;

    ret = path_lookup(path, &resolved);
    CHECK(ret == 0, "fs: path lookup failed", return NULL;);
    CHECK(resolved.dentry != NULL && resolved.dentry->d_inode != NULL, "fs: negative dentry", return NULL;);

    file = kmalloc(sizeof(*file));
    CHECK(file != NULL, "fs: alloc file failed", return NULL;);
    memset(file, 0, sizeof(*file));

    file->f_path = resolved;
    file->f_inode = resolved.dentry->d_inode;
    file->f_op = file->f_inode->i_fop;
    file->f_flags = flags;
    atomic_set(&file->f_count, 1);


    if (file->f_op != NULL && file->f_op->open != NULL) {
        ret = file->f_op->open(file->f_inode, file);
        CHECK(ret == 0, "fs: file open op failed", kfree(file); return NULL;);
    }

    return file;
}

void filp_close(struct file *file) {
    if (file != NULL) {
        if (file->f_path.dentry != NULL) {
            dput(file->f_path.dentry);
        }
        if (file->f_inode != NULL) {
            iput(file->f_inode);
        }
        kfree(file);
    }
}

ssize_t kernel_read(struct file *file, char *buf, size_t len) {
    CHECK(file != NULL, "fs: invalid file", return -1;);
    CHECK(file->f_op != NULL && file->f_op->read != NULL, "fs: read op missing", return -1;);
    return file->f_op->read(file, buf, len, &file->f_pos);
}

ssize_t kernel_write(struct file *file, const char *buf, size_t len) {
    CHECK(file != NULL, "fs: invalid file", return -1;);
    CHECK(file->f_op != NULL && file->f_op->write != NULL, "fs: write op missing", return -1;);
    return file->f_op->write(file, buf, len, &file->f_pos);
}

static inline void __set_close_on_exec(int fd, struct fdtable *fdt)
{
	__set_bit(fd, fdt->close_on_exec);
}

static inline void __clear_close_on_exec(int fd, struct fdtable *fdt)
{
	__clear_bit(fd, fdt->close_on_exec);
}

static inline void __set_open_fd(int fd, struct fdtable *fdt)
{
	__set_bit(fd, fdt->open_fds);
}

static inline void __clear_open_fd(int fd, struct fdtable *fdt)
{
	__clear_bit(fd, fdt->open_fds);
}


static int count_open_files(struct fdtable *fdt)
{
	int size = fdt->max_fds;
	int i;

	/* Find the last open fd */
	for (i = size / BITS_PER_LONG; i > 0; ) {
		if (fdt->open_fds[--i])
			break;
	}
	i = (i + 1) * BITS_PER_LONG;
	return i;
}


/*
 * Allocate a new files structure and copy contents from the
 * passed in files structure.
 * errorp will be valid only when the returned files_struct is NULL.
 */
struct files_struct *dup_fd(struct files_struct *oldf, int *errorp) {
	struct files_struct *newf;
	struct file **old_fds, **new_fds;
	int open_files, size, i;
	struct fdtable *old_fdt, *new_fdt;

	newf = kmalloc(sizeof(struct files_struct));
	if (!newf) {
        *errorp = -ENOMEM;
        goto out;
    }


	atomic_set(&newf->refcount, 1);

	spin_lock_init(&newf->file_lock);
	newf->next_fd = 0;
	new_fdt = &newf->fdtab;
	new_fdt->max_fds = MAX_OPEN_FILES_NUM;
	new_fdt->close_on_exec = newf->close_on_exec_init;
	new_fdt->open_fds = newf->open_fds_init;
	new_fdt->fd = &newf->fd_array[0];

	spin_lock(&oldf->file_lock);
	old_fdt = &oldf->fdtab;
	open_files = count_open_files(old_fdt);

	old_fds = old_fdt->fd;
	new_fds = new_fdt->fd;

	memcpy(new_fdt->open_fds, old_fdt->open_fds, open_files / 8);
	memcpy(new_fdt->close_on_exec, old_fdt->close_on_exec, open_files / 8);

	for (i = open_files; i != 0; i--) {
		struct file *f = *old_fds++;
		if (f) {
			get_file(f);
		} else {
			/*
			 * The fd may be claimed in the fd bitmap but not yet
			 * instantiated in the files array if a sibling thread
			 * is partway through open().  So make sure that this
			 * fd is available to the new process.
			 */
			__clear_open_fd(open_files - i, new_fdt);
		}
        *new_fds = f;
        new_fds++;
	}
	spin_unlock(&oldf->file_lock);

	/* compute the remainder to be cleared */
	size = (new_fdt->max_fds - open_files) * sizeof(struct file *);

	/* This is long word aligned thus could use a optimized version */
	memset(new_fds, 0, size);

	if (new_fdt->max_fds > open_files) {
		int left = (new_fdt->max_fds - open_files) / 8;
		int start = open_files / BITS_PER_LONG;

		memset(&new_fdt->open_fds[start], 0, left);
		memset(&new_fdt->close_on_exec[start], 0, left);
	} else {
        *errorp = -EMFILE;
    }

	return newf;

out:
	return NULL;
}

struct files_struct *get_files_struct(struct task_struct *task) {
	struct files_struct *files;

	spin_lock(&task->lock);
	files = task->files;
	if (files)
		atomic_inc(&files->refcount);
	spin_lock(&task->lock);

	return files;
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
					filp_close(file);
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
unsigned long find_next_zero_bit(const unsigned long *addr,
                                 unsigned long size,
                                 unsigned long offset)
{
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
            unsigned long bit = __builtin_ctz(~(addr[k] | mask));
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
            unsigned long bit = __builtin_ctz(~addr[k]);
            unsigned long pos = k * BITS_PER_LONG + bit;
            return pos < size ? pos : size;
        }
    }

    // 没找到，返回 size（表示满了）
    return size;
}


/*
 * allocate a file descriptor, mark it busy.
 */
int __alloc_fd(struct files_struct *files,
	       unsigned start, unsigned end, unsigned flags)
{
	unsigned int fd;
	int error;
	struct fdtable *fdt;

	spin_lock(&files->file_lock);
	fdt = &files->fdtab;
	fd = start;
	if (fd < files->next_fd)
		fd = files->next_fd;

	if (fd < fdt->max_fds)
		fd = find_next_zero_bit(fdt->open_fds, fdt->max_fds, fd);

	/*
	 * N.B. For clone tasks sharing a files structure, this test
	 * will limit the total number of files that can be opened.
	 */
	error = -EMFILE;
	if (fd >= end)
		goto out;

	__set_open_fd(fd, fdt);
	if (flags & O_CLOEXEC)
		__set_close_on_exec(fd, fdt);
	else
		__clear_close_on_exec(fd, fdt);
	error = fd;

out:
	spin_unlock(&files->file_lock);
	return error;
}

static void __put_unused_fd(struct files_struct *files, unsigned int fd) {
	struct fdtable *fdt = &files->fdtab;
	__clear_open_fd(fd, fdt);
	if (fd < files->next_fd)
		files->next_fd = fd;
}

static int alloc_fd(unsigned start, unsigned flags) {
	return __alloc_fd(current->files, start, MAX_OPEN_FILES_NUM, flags);
}

int __close_fd(struct files_struct *files, unsigned fd) {
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
	filp_close(file);
    return 0;

out_unlock:
	spin_unlock(&files->file_lock);
	return -EBADF;
}

void do_close_on_exec(struct files_struct *files) {
	unsigned i;
	struct fdtable *fdt;

	/* exec unshares first */
	spin_lock(&files->file_lock);
	for (i = 0; ; i++) {
		unsigned long set;
		unsigned fd = i * BITS_PER_LONG;
		fdt = &files->fdtab;
		if (fd >= fdt->max_fds)
			break;
		set = fdt->close_on_exec[i];
		if (!set)
			continue;
		fdt->close_on_exec[i] = 0;
		for ( ; set ; fd++, set >>= 1) {
			struct file *file;
			if (!(set & 1))
				continue;
			file = fdt->fd[fd];
			if (!file)
				continue;
			fdt->fd[fd] =  NULL;
			__put_unused_fd(files, fd);
			spin_unlock(&files->file_lock);
			filp_close(file);
			spin_lock(&files->file_lock);
		}

	}
	spin_unlock(&files->file_lock);
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