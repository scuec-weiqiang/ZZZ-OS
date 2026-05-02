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

static inline void put_file(struct file *f) {
	if (atomic_read(&f->f_count) > 0) {
		atomic_dec(&f->f_count);
	}
}

struct file *filp_open(const char *path, u32 flags);
void filp_close(struct file *file);
ssize_t kernel_read(struct file *file, char *buf, size_t len);
ssize_t kernel_read_at(struct file *file, loff_t pos, char *buf, size_t len);
ssize_t kernel_write(struct file *file, const char *buf, size_t len);

int alloc_fd(unsigned start, unsigned flags);
int close_fd(unsigned fd);

static inline bool close_on_exec(int fd, const struct fdtable *fdt) {
	return test_bit(fd, fdt->close_on_exec);
}

static inline bool fd_is_open(int fd, const struct fdtable *fdt) {
	return test_bit(fd, fdt->open_fds);
}

struct file *fd_get_file(unsigned int fd);
void fd_put_file(struct file *file);

void attach_fd(unsigned int fd, struct file *file);
struct files_struct *dup_fd(struct files_struct *oldf);
void put_files_struct(struct files_struct *files);
int setup_stdio(const char *path);

extern struct files_struct init_files;

int alloc_files_struct_init(void);
struct files_struct *alloc_files_struct(void);
void free_files_struct(struct files_struct *files);

void do_close_on_exec(struct files_struct *files);
void set_close_on_exec(unsigned int fd, int flag);

struct files_struct *get_files_struct(struct task_struct *task);
void put_files_struct(struct files_struct *files);

#endif
