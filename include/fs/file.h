#ifndef FS2_FILE_H
#define FS2_FILE_H

#include <fs/types.h>
#include <os/atomic.h>

#define O_ACCMODE	00000003
#define O_RDONLY	00000000
#define O_WRONLY	00000001
#define O_RDWR		00000002
#define O_CREAT		00000100	/* not fcntl */
#define O_EXCL		00000200	/* not fcntl */
#define O_APPEND	00002000
#define O_CLOEXEC	02000000	/* set close_on_exec */

static inline struct file *get_file(struct file *f) {
	atomic_inc(&f->f_count);
	return f;
}

struct file *filp_open(const char *path, uint32_t flags);
void filp_close(struct file *file);
ssize_t kernel_read(struct file *file, char *buf, size_t len);
ssize_t kernel_write(struct file *file, const char *buf, size_t len);
struct files_struct *dup_fd(struct files_struct *oldf, int *errorp);

extern struct files_struct init_files ;

#endif
