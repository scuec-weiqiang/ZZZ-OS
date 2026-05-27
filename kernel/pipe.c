#include <fs/file.h>
#include <os/pipe.h>
#include <os/string.h>
#include <mm/slab.h>
#include <os/err.h>
#include <os/minmax.h>
#include <os/uaccess.h>
#include <asm/ptrace.h>
#include <mm/buddy.h>
#include <os/sched.h>

static struct kmem_cache *pipe_inode_info_kcache = NULL;

int alloc_pipe_inode_info_init(void) {
    pipe_inode_info_kcache = kmem_cache_create("pipe_inode_info",
                                               sizeof(struct pipe_inode_info),
                                                8);
    if (!pipe_inode_info_kcache)
        return -ENOMEM;

    return 0;
}

struct pipe_inode_info *alloc_pipe_inode_info(void) {
    if (pipe_inode_info_kcache == NULL)
        return ERR_PTR(-ENOMEM);

    struct pipe_inode_info *pipe = kmem_cache_alloc(pipe_inode_info_kcache);
    if (!pipe)
        return ERR_PTR(-ENOMEM);

    memset(pipe, 0, sizeof(*pipe));
    init_waitqueue_head(&pipe->read_wait);
    init_waitqueue_head(&pipe->write_wait);
    spin_lock_init(&pipe->lock);
    pipe->buf = alloc_pages_kva(1);

    if (!pipe->buf) {
        kmem_cache_free(pipe);
        return ERR_PTR(-ENOMEM);
    }
    return pipe;
}

void free_pipe_inode_info(struct pipe_inode_info *pipe) {
    if (pipe) {
        if (pipe->buf)
            free_pages_kva(pipe->buf);
        kmem_cache_free(pipe);
    }
}

// 读端一次最多读 PIPE_BUF_SIZE 字节，写端一次最多写 PIPE_BUF_SIZE 字节
static ssize_t __pipe_read(struct pipe_inode_info *pipe, char *buf, size_t len) {
    size_t bytes_to_read;
    size_t i;

    if (len == 0)
        return 0;

    while (1) {
        spin_lock(&pipe->lock);
        if (pipe->count > 0) {
            break;
        }
        if (pipe->writers == 0) {
            spin_unlock(&pipe->lock);
            return 0;
        }
        spin_unlock(&pipe->lock);
        sleep_on(&pipe->read_wait);
    }

    bytes_to_read = min(len, pipe->count);
    for (i = 0; i < bytes_to_read; i++) {
        buf[i] = pipe->buf[(pipe->tail + i) % PIPE_BUF_SIZE];
    }

    pipe->tail += bytes_to_read;
    pipe->tail %= PIPE_BUF_SIZE;
    pipe->count -= bytes_to_read;
    spin_unlock(&pipe->lock);

    wake_up_one(&pipe->write_wait);
    return (ssize_t)bytes_to_read;
}

static ssize_t __pipe_write(struct pipe_inode_info *pipe, const char *buf, size_t len) {
    size_t space_available;
    size_t bytes_to_write;
    size_t i;

    if (len == 0)
        return 0;

    while (1) {
        spin_lock(&pipe->lock);
        if (pipe->readers == 0) {
            spin_unlock(&pipe->lock);
            return -EPIPE;
        }
        if (pipe->count < PIPE_BUF_SIZE) {
            break;
        }
        spin_unlock(&pipe->lock);
        sleep_on(&pipe->write_wait);
    }

    space_available = PIPE_BUF_SIZE - pipe->count;
    bytes_to_write = min(len, space_available);

    for (i = 0; i < bytes_to_write; i++) {
        pipe->buf[(pipe->head + i) % PIPE_BUF_SIZE] = buf[i];
    }

    pipe->head += bytes_to_write;
    pipe->head %= PIPE_BUF_SIZE;
    pipe->count += bytes_to_write;
    spin_unlock(&pipe->lock);

    wake_up_one(&pipe->read_wait);
    return (ssize_t)bytes_to_write;
}

static ssize_t pipe_write(struct file *file, const char *buf, size_t len, loff_t *ppos) {
    (void)ppos;
    struct pipe_inode_info *pipe = file->private_data;
    return __pipe_write(pipe, buf, len);
}

static ssize_t pipe_read(struct file *file, char *buf, size_t len, loff_t *ppos) {
    (void)ppos;
    struct pipe_inode_info *pipe = file->private_data;
    return __pipe_read(pipe, buf, len);
}

int pipe_release(struct inode *inode, struct file *file) {
    int free_pipe;
    struct pipe_inode_info *p = file->private_data;
    (void)inode;

    if (p == NULL)
        return 0;

    spin_lock(&p->lock);

    if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
        if (p->writers > 0)
            p->writers--;
        if (p->writers == 0)
            wake_up_all(&p->read_wait);
    } else {
        if (p->readers > 0)
            p->readers--;
        if (p->readers == 0)
            wake_up_all(&p->write_wait);
    }

    free_pipe = (p->readers == 0 && p->writers == 0);

    spin_unlock(&p->lock);

    if (free_pipe)
        free_pipe_inode_info(p);

    return 0;
}

static struct file_operations pipe_read_fops = {
    .read = pipe_read,
    .release = pipe_release,
};

static struct file_operations pipe_write_fops = {
    .write = pipe_write,
    .release = pipe_release,
};

static void pipe_setup_file(struct file *file, const struct file_operations *fops,
                            int flags, struct pipe_inode_info *pipe) {
    memset(file, 0, sizeof(*file));
    file->f_op = fops;
    file->f_flags = flags;
    file->private_data = pipe;
    atomic_set(&file->f_count, 1);
}

long sys_pipe(struct pt_regs *ctx) {
    uintptr_t user_fds = ctx->r[0];
    int fd0, fd1;
    int fds[2];
    struct pipe_inode_info *pipe;
    struct file *rfile = NULL;
    struct file *wfile = NULL;

    if (user_fds == 0)
        return -EFAULT;

    pipe = alloc_pipe_inode_info();
    if (IS_ERR(pipe)) {
        return PTR_ERR(pipe);
    }

    pipe->readers = 0;
    pipe->writers = 0;

    rfile = alloc_file();
    if (rfile == NULL) {
        free_pipe_inode_info(pipe);
        return -ENOMEM;
    }
    wfile = alloc_file();
    if (wfile == NULL) {
        free_file(rfile);
        free_pipe_inode_info(pipe);
        return -ENOMEM;
    }

    pipe_setup_file(rfile, &pipe_read_fops, O_RDONLY, pipe);
    pipe_setup_file(wfile, &pipe_write_fops, O_WRONLY, pipe);

    fd0 = alloc_fd(0, 0);
    if (fd0 < 0) {
        free_file(rfile);
        free_file(wfile);
        free_pipe_inode_info(pipe);
        return fd0;
    }
    attach_fd(fd0, rfile);
    spin_lock(&pipe->lock);
    pipe->readers = 1;
    spin_unlock(&pipe->lock);
    rfile = NULL;

    fd1 = alloc_fd(0, 0);
    if (fd1 < 0) {
        close_fd((unsigned)fd0);
        free_file(wfile);
        return fd1;
    }
    attach_fd(fd1, wfile);
    spin_lock(&pipe->lock);
    pipe->writers = 1;
    spin_unlock(&pipe->lock);
    wfile = NULL;

    fds[0] = fd0;
    fds[1] = fd1;

    if (copy_to_user((char *)user_fds, (char *)fds, sizeof(fds)) != 0) {
        close_fd((unsigned)fd0);
        close_fd((unsigned)fd1);
        return -EFAULT;
    }

    return 0;
}

