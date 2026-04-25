#include <asm/ptrace.h>
#include <fs/file.h>
#include <fs/namei.h>
#include <os/err.h>
#include <os/console.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/mm.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/syscall.h>
#include <os/syscall_num.h>

#define SYSCALL_PATH_MAX 256
#define SYSCALL_IO_CHUNK 512

static int copy_user_string(char *dst, size_t dst_len, uintptr_t user_ptr)
{
    size_t i;

    if (dst == NULL || dst_len == 0 || user_ptr == 0 || current->mm == NULL) {
        return -EINVAL;
    }

    if (copyin(current->mm->pgdir, dst, user_ptr, dst_len) < 0) {
        return -EFAULT;
    }

    for (i = 0; i < dst_len; i++) {
        if (dst[i] == '\0') {
            return 0;
        }
    }

    dst[dst_len - 1] = '\0';
    return -ENAMETOOLONG;
}

static struct file *sys_fdget(int fd)
{
    struct files_struct *files;
    struct fdtable *fdt;

    if (fd < 0) {
        return NULL;
    }

    files = current->files;
    if (files == NULL) {
        return NULL;
    }

    fdt = &files->fdtab;
    if (fd >= fdt->max_fds) {
        return NULL;
    }

    return fdt->fd[fd];
}

static long sys_print(uintptr_t user_buf, size_t len)
{
    char *kbuf;
    size_t chunk;
    size_t done;

    if (len == 0) {
        return 0;
    }
    if (current->mm == NULL) {
        return -EFAULT;
    }

    kbuf = kmalloc(SYSCALL_IO_CHUNK + 1);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    done = 0;
    while (done < len) {
        chunk = len - done;
        if (chunk > SYSCALL_IO_CHUNK) {
            chunk = SYSCALL_IO_CHUNK;
        }

        if (copyin(current->mm->pgdir, kbuf, user_buf + done, chunk) < 0) {
            kfree(kbuf);
            return -EFAULT;
        }

        kbuf[chunk] = '\0';
        console_puts(kbuf);
        done += chunk;
    }
    console_flush();
    kfree(kbuf);
    return (long)len;
}

static long sys_getpid(void)
{
    return current->pid;
}

static long sys_yield(void)
{
    yield();
    return 0;
}

static long sys_open(uintptr_t user_path, int flags)
{
    char path[SYSCALL_PATH_MAX];
    struct file *file;
    int fd;

    if (copy_user_string(path, sizeof(path), user_path) < 0) {
        return -EFAULT;
    }

    file = filp_open(path, (uint32_t)flags);
    if (IS_ERR(file)) {
        if ((flags & O_CREAT) == 0) {
            return PTR_ERR(file);
        }

        if (vfs_create(path, 0644) == NULL) {
            return -ENOENT;
        }

        file = filp_open(path, (uint32_t)flags);
        if (IS_ERR(file)) {
            return PTR_ERR(file);
        }
    }

    fd = __alloc_fd(current->files, 0, MAX_OPEN_FILES_NUM, (unsigned)flags);
    if (fd < 0) {
        filp_close(file);
        return fd;
    }

    current->files->fdtab.fd[fd] = file;
    return fd;
}

static long sys_close(int fd)
{
    return __close_fd(current->files, (unsigned)fd);
}

static long sys_read(int fd, uintptr_t user_buf, size_t len)
{
    struct file *file;
    char *kbuf;
    ssize_t ret;

    if (len == 0) {
        return 0;
    }

    file = sys_fdget(fd);
    if (file == NULL) {
        return -EBADF;
    }

    kbuf = kmalloc(len);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    ret = kernel_read(file, kbuf, len);
    if (ret >= 0) {
        if (copyout(current->mm->pgdir, user_buf, kbuf, (size_t)ret) < 0) {
            kfree(kbuf);
            return -EFAULT;
        }
    }

    kfree(kbuf);
    return ret;
}

static long sys_write(int fd, uintptr_t user_buf, size_t len)
{
    struct file *file;
    char *kbuf;
    ssize_t ret;

    if (fd == 1 || fd == 2) {
        return sys_print(user_buf, len);
    }

    if (len == 0) {
        return 0;
    }

    file = sys_fdget(fd);
    if (file == NULL) {
        return -EBADF;
    }

    kbuf = kmalloc(len);
    if (kbuf == NULL) {
        return -ENOMEM;
    }

    if (copyin(current->mm->pgdir, kbuf, user_buf, len) < 0) {
        kfree(kbuf);
        return -EFAULT;
    }

    ret = kernel_write(file, kbuf, len);
    kfree(kbuf);
    return ret;
}

static long sys_creat(uintptr_t user_path, int mode)
{
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0) {
        return -EFAULT;
    }

    if (vfs_create(path, (uint16_t)mode) == NULL) {
        return -EIO;
    }

    return sys_open(user_path, O_WRONLY);
}

static long sys_mkdir(uintptr_t user_path, int mode)
{
    char path[SYSCALL_PATH_MAX];

    if (copy_user_string(path, sizeof(path), user_path) < 0) {
        return -EFAULT;
    }

    if (vfs_mkdir(path, (uint16_t)mode) == NULL) {
        return -EIO;
    }

    return 0;
}

void do_syscall(struct pt_regs *ctx)
{
    long ret;
    uint32_t nr;

    if (ctx == NULL) {
        return;
    }

    nr = ctx->r[7];
    current_thread_info()->syscall = nr;
    ret = -ENOSYS;

    switch (nr) {
    case SYSCALL_PRINT:
        ret = sys_print(ctx->r[0], ctx->r[1]);
        break;
    case SYSCALL_YIELD:
        ret = sys_yield();
        break;
    case SYSCALL_OPEN:
        ret = sys_open(ctx->r[0], (int)ctx->r[1]);
        break;
    case SYSCALL_CLOSE:
        ret = sys_close((int)ctx->r[0]);
        break;
    case SYSCALL_READ:
        ret = sys_read((int)ctx->r[0], ctx->r[1], ctx->r[2]);
        break;
    case SYSCALL_WRITE:
        ret = sys_write((int)ctx->r[0], ctx->r[1], ctx->r[2]);
        break;
    case SYSCALL_CREAT:
        ret = sys_creat(ctx->r[0], (int)ctx->r[1]);
        break;
    case SYSCALL_MKDIR:
        ret = sys_mkdir(ctx->r[0], (int)ctx->r[1]);
        break;
    case SYSCALL_GETPID:
        ret = sys_getpid();
        break;
    case SYSCALL_EXIT:
        do_exit((int)ctx->r[0]);
        break;
    default:
        break;
    }

    ctx->r[0] = (reg_t)ret;
}
