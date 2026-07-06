#include <asm/ptrace.h>
#include <asm/syscall_num.h>
#include <fs/dcache.h>
#include <fs/file.h>
#include <fs/namei.h>
#include <fs/types.h>
#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/printk.h>
#include <os/rand.h>
#include <os/sched.h>
#include <os/syscall.h>
#include <os/timekeeping.h>
#include <os/uaccess.h>

#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define AT_SYMLINK_NOFOLLOW 0x100
#define AT_EACCESS 0x200
#define AT_EMPTY_PATH 0x1000
#define RLIM_INFINITY (~0UL)

extern long sys_open(struct pt_regs *ctx);
extern long sys_stat(struct pt_regs *ctx);
extern long sys_access(struct pt_regs *ctx);
extern long sys_mkdir(struct pt_regs *ctx);
extern long sys_rmdir(struct pt_regs *ctx);
extern long sys_unlink(struct pt_regs *ctx);
extern long sys_pipe(struct pt_regs *ctx);
extern long sys_dup2(struct pt_regs *ctx);
extern long sys_waitpid(struct pt_regs *ctx);
extern long sys_fork(struct pt_regs *ctx);

struct linux_rlimit {
    unsigned long rlim_cur;
    unsigned long rlim_max;
};

static int syscall_copy_user_string(char *dst, size_t dst_len, uintptr_t user_ptr) {
    size_t i;

    if (!dst || dst_len == 0 || user_ptr == 0 || current->mm == NULL)
        return -EINVAL;

    for (i = 0; i < dst_len; i++) {
        if (copy_from_user(&dst[i], (const char *)user_ptr + i, 1) < 0)
            return -EFAULT;
        if (dst[i] == '\0')
            return 0;
    }

    dst[dst_len - 1] = '\0';
    return -ENAMETOOLONG;
}

const syscall_fn_t syscall_table[SYSCALL_MAX] = {
#define X(nr, name) [nr] = sys_##name,
    SYSCALL_LIST
#undef X
};

long sys_openat(struct pt_regs *ctx) {
    struct pt_regs open_ctx = *ctx;

    open_ctx.r[0] = ctx->r[1];
    open_ctx.r[1] = ctx->r[2];
    return sys_open(&open_ctx);
}

long sys_newfstatat(struct pt_regs *ctx) {
    struct pt_regs stat_ctx = *ctx;

    stat_ctx.r[0] = ctx->r[1];
    stat_ctx.r[1] = ctx->r[2];
    return sys_stat(&stat_ctx);
}

long sys_faccessat(struct pt_regs *ctx) {
    struct pt_regs access_ctx = *ctx;

    access_ctx.r[0] = ctx->r[1];
    access_ctx.r[1] = ctx->r[2];
    return sys_access(&access_ctx);
}

long sys_faccessat2(struct pt_regs *ctx) {
    unsigned int flags = (unsigned int)ctx->r[3];

    if (flags & ~(AT_SYMLINK_NOFOLLOW | AT_EACCESS | AT_EMPTY_PATH))
        return -EINVAL;

    return sys_faccessat(ctx);
}

long sys_mkdirat(struct pt_regs *ctx) {
    struct pt_regs mkdir_ctx = *ctx;

    mkdir_ctx.r[0] = ctx->r[1];
    mkdir_ctx.r[1] = ctx->r[2];
    return sys_mkdir(&mkdir_ctx);
}

long sys_unlinkat(struct pt_regs *ctx) {
    struct pt_regs unlink_ctx = *ctx;

    unlink_ctx.r[0] = ctx->r[1];
    if (ctx->r[2] & 0x200)
        return sys_rmdir(&unlink_ctx);

    return sys_unlink(&unlink_ctx);
}

long sys_pipe2(struct pt_regs *ctx) {
    if (ctx->r[1] != 0)
        return -EINVAL;

    return sys_pipe(ctx);
}

long sys_dup3(struct pt_regs *ctx) {
    if (ctx->r[2] != 0)
        return -EINVAL;

    return sys_dup2(ctx);
}

long sys_socket(struct pt_regs *ctx) {
    return -EAFNOSUPPORT;
}

long sys_readlinkat(struct pt_regs *ctx) {
    const char *pathname = (const char *)ctx->r[1];
    char *user_buf = (char *)ctx->r[2];
    size_t bufsiz = ctx->r[3];
    char path_buf[256];
    char *kbuf;
    ssize_t ret;

    if (bufsiz == 0)
        return -EINVAL;

    if (syscall_copy_user_string(path_buf, sizeof(path_buf), (uintptr_t)pathname) < 0)
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

long sys_symlinkat(struct pt_regs *ctx) {
    const char *target = (const char *)ctx->r[0];
    const char *linkpath = (const char *)ctx->r[2];
    char target_buf[256];
    char link_buf[256];
    struct dentry *dentry;

    if (syscall_copy_user_string(target_buf, sizeof(target_buf), (uintptr_t)target) < 0)
        return -EFAULT;

    if (syscall_copy_user_string(link_buf, sizeof(link_buf), (uintptr_t)linkpath) < 0)
        return -EFAULT;

    dentry = vfs_symlink(link_buf, target_buf);
    if (dentry == NULL)
        return -EIO;

    dput(dentry);
    return 0;
}

long sys_renameat2(struct pt_regs *ctx) {
    const char *old_path = (const char *)ctx->r[1];
    const char *new_path = (const char *)ctx->r[3];
    unsigned int flags = ctx->r[4];
    char old_buf[256];
    char new_buf[256];

    if (flags != 0)
        return -EINVAL;

    if (syscall_copy_user_string(old_buf, sizeof(old_buf), (uintptr_t)old_path) < 0)
        return -EFAULT;

    if (syscall_copy_user_string(new_buf, sizeof(new_buf), (uintptr_t)new_path) < 0)
        return -EFAULT;

    return vfs_rename(old_buf, new_buf);
}

long sys_set_tid_address(struct pt_regs *ctx) {
    current->clear_child_tid = (int __user *)ctx->r[0];
    return current->pid;
}

long sys_set_robust_list(struct pt_regs *ctx) {
    return 0;
}

long sys_rt_sigprocmask(struct pt_regs *ctx) {
    void *oldset = (void *)ctx->r[2];
    size_t sigsetsize = ctx->r[3];
    u64 empty = 0;

    if (oldset == NULL)
        return 0;

    if (sigsetsize > sizeof(empty))
        sigsetsize = sizeof(empty);

    if (copy_to_user(oldset, (char *)&empty, sigsetsize) < 0)
        return -EFAULT;

    return 0;
}

long sys_wait4(struct pt_regs *ctx) {
    return sys_waitpid(ctx);
}

long sys_clock_gettime(struct pt_regs *ctx) {
    int clockid = (int)ctx->r[0];
    timespec_t ts;
    u64 now;

    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC)
        return -EINVAL;

    now = monotonic_ns();
    ts.tv_sec = now / NSEC_PER_SEC;
    ts.tv_nsec = now % NSEC_PER_SEC;

    if (copy_to_user((char *)ctx->r[1], (char *)&ts, sizeof(ts)) < 0)
        return -EFAULT;

    return 0;
}

long sys_clock_getres(struct pt_regs *ctx) {
    int clockid = (int)ctx->r[0];
    timespec_t ts = {
        .tv_sec = 0,
        .tv_nsec = 1,
    };

    if (clockid != CLOCK_REALTIME && clockid != CLOCK_MONOTONIC)
        return -EINVAL;

    if (ctx->r[1] == 0)
        return 0;

    if (copy_to_user((char *)ctx->r[1], (char *)&ts, sizeof(ts)) < 0)
        return -EFAULT;

    return 0;
}

long sys_getuid(struct pt_regs *ctx) {
    /*
     * We do not have credentials yet.  Return a non-root uid so archive
     * tools do not try to preserve ownership through chown/lchown.
     */
    return 1000;
}

long sys_geteuid(struct pt_regs *ctx) {
    return sys_getuid(ctx);
}

long sys_getppid(struct pt_regs *ctx) {
    if (current->parent == NULL)
        return 0;

    return current->parent->pid;
}

long sys_fchmodat(struct pt_regs *ctx) {
    const char *pathname = (const char *)ctx->r[1];
    mode_t mode = (mode_t)ctx->r[2];
    char path_buf[256];
    struct path path = {0};
    struct inode *inode;
    int ret;

    if (syscall_copy_user_string(path_buf, sizeof(path_buf), (uintptr_t)pathname) < 0)
        return -EFAULT;

    ret = path_lookup(path_buf, &path);
    if (ret < 0)
        return ret;

    inode = path.dentry->d_inode;
    if (inode == NULL) {
        ret = -ENOENT;
        goto out;
    }

    inode->i_mode = (inode->i_mode & S_IFMT) | (mode & 07777);
    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->write_inode != NULL) {
        ret = inode->i_sb->s_op->write_inode(inode);
    } else {
        ret = 0;
    }

out:
    path_put(&path);
    return ret;
}

long sys_utimensat(struct pt_regs *ctx) {
    const char *pathname = (const char *)ctx->r[1];
    const timespec_t *user_times = (const timespec_t *)ctx->r[2];
    int flags = (int)ctx->r[3];
    char path_buf[256];
    struct path path = {0};
    struct inode *inode;
    timespec_t times[2];
    timespec_t now;
    int ret;

    if (flags & ~0x100)
        return -EINVAL;

    if (syscall_copy_user_string(path_buf, sizeof(path_buf), (uintptr_t)pathname) < 0)
        return -EFAULT;

    ret = (flags & 0x100) ? path_lookup_nofollow(path_buf, &path) : path_lookup(path_buf, &path);
    if (ret < 0)
        return ret;

    inode = path.dentry->d_inode;
    if (inode == NULL) {
        ret = -ENOENT;
        goto out;
    }

    if (user_times != NULL) {
        if (copy_from_user((char *)times, (char *)user_times, sizeof(times)) < 0) {
            ret = -EFAULT;
            goto out;
        }
        inode->i_atime = times[0];
        inode->i_mtime = times[1];
    } else {
        u64 ns = monotonic_ns();
        now.tv_sec = ns / NSEC_PER_SEC;
        now.tv_nsec = ns % NSEC_PER_SEC;
        inode->i_atime = now;
        inode->i_mtime = now;
    }

    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->write_inode != NULL) {
        ret = inode->i_sb->s_op->write_inode(inode);
    } else {
        ret = 0;
    }

out:
    path_put(&path);
    return ret;
}

long sys_riscv_hwprobe(struct pt_regs *ctx) {
    return -ENOSYS;
}

long sys_prlimit64(struct pt_regs *ctx) {
    unsigned int resource = ctx->r[1];
    const struct linux_rlimit *new_limit = (const struct linux_rlimit *)ctx->r[2];
    struct linux_rlimit *old_limit = (struct linux_rlimit *)ctx->r[3];
    struct linux_rlimit limit = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };

    if (new_limit != NULL)
        return -EPERM;

    if (resource == 3) {
        limit.rlim_cur = 8UL * 1024 * 1024;
        limit.rlim_max = 8UL * 1024 * 1024;
    } else if (resource == 7) {
        limit.rlim_cur = 256;
        limit.rlim_max = 256;
    }

    if (old_limit != NULL && copy_to_user((char *)old_limit, (char *)&limit, sizeof(limit)) < 0)
        return -EFAULT;

    return 0;
}

long sys_getrandom(struct pt_regs *ctx) {
    char *buf = (char *)ctx->r[0];
    size_t buflen = ctx->r[1];
    size_t done = 0;

    while (done < buflen) {
        unsigned int value = (unsigned int)rand() ^ (unsigned int)monotonic_ns();
        size_t chunk = buflen - done;

        if (chunk > sizeof(value))
            chunk = sizeof(value);

        if (copy_to_user(buf + done, (char *)&value, chunk) < 0)
            return -EFAULT;

        done += chunk;
    }

    return done;
}

long sys_rseq(struct pt_regs *ctx) {
    return -ENOSYS;
}

void do_syscall(struct pt_regs *ctx) {
    long ret = -ENOSYS;
    u32 nr;

    if (ctx == NULL)
        return;

    nr = ctx->r[7];
    // dprintk("syscall: nr=%u\n", nr);
    current_thread_info()->syscall = nr;
    if (nr < SYSCALL_MAX && syscall_table[nr] != NULL) {
        ret = syscall_table[nr](ctx);
    } else {
        dprintk("syscall: nr=%u not implemented\n", nr);
    }

    if (nr == SYSCALL_sigreturn) {
        //
    } else {
        ctx->r[0] = (reg_t)ret;
    }

#ifdef SYS_TRACE_ENABLE
    printk("[syscall %d] %s called by pid=%d ret=%ld\n", nr, syscall_names[nr], current->pid, ret);
#endif
}
