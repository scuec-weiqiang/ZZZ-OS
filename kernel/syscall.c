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

const syscall_fn_t syscall_table[SYSCALL_MAX] = {
#define X(nr, name) [nr] = sys_##name,
    SYSCALL_LIST
#undef X
};

long sys_socket(struct pt_regs *ctx) {
    return -EAFNOSUPPORT;
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

    if (copy_user_string(path_buf, sizeof(path_buf), (uintptr_t)pathname) < 0)
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
