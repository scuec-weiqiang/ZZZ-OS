/**
 * @FilePath: /ZZZ-OS/kernel/syscall.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-02 18:11:27
 * @LastEditTime: 2025-10-29 22:29:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/syscall.h>
#include <os/syscall_num.h>

#include <asm/riscv.h>
#include <os/printk.h>
#include <os/proc.h>
#include <os/sched.h>
#include <fs/vfs.h>
#include <os/malloc.h>

int sys_printf(struct trap_frame *ctx)
{
    char *s = malloc(ctx->a1); // 获取字符串指针
    copyin(current_proc()->pgd, s, (uintptr_t)ctx->a0, (uintptr_t)ctx->a1);
    int n = printk("%s", s);
    free(s);
    return n;
}

int sys_yield(struct trap_frame *ctx)
{
    // printk("sys_yield called by %xu\n", ctx->a0);
    yield(); // 调用调度函数，切换到其他线程
    return 0;
}

int sys_open(struct trap_frame *ctx)
{
    char path[128];
    char *s = path;
    char *d = (char*)ctx->a0;
    struct proc *p = current_proc();
    for(;;)
    {
        copyin(p->pgd, s, (uintptr_t)d, 1);
        if(*s == '\0' || s - path >= 127)
        {
            break;
        }
        s++;
        d++;
    }
    int flags = ctx->a1;
    struct file *file = open(path, flags);
    return alloc_fd(p, file);
}

int sys_close(struct trap_frame *ctx)
{
    struct proc *p = current_proc();
    int fd = ctx->a0;
    close(p->fd_table[fd]);
    free_fd(p, fd);
    return 0;
}

int sys_read(struct trap_frame *ctx)
{
    struct proc *p = current_proc();
    int fd = ctx->a0;
    char *buf = (char*)ctx->a1;
    size_t count = ctx->a2;
    if(fd < 0 || fd >= 256 || p->fd_table[fd] == NULL)
    {
        return -1;
    }
    char *kbuf = (char*)malloc(count);
    ssize_t ret = read(p->fd_table[fd], kbuf, count);
    if(ret >= 0)
    {
        copyout(p->pgd, (uintptr_t)buf, kbuf, ret);
    }
    free(kbuf);
    return ret;
}

int sys_write(struct trap_frame *ctx)
{
    struct proc *p = current_proc();
    int fd = ctx->a0;
    char *buf = (char*)ctx->a1;
    size_t count = ctx->a2;
    if(fd < 0 || fd >= 256 || p->fd_table[fd] == NULL)
    {
        return -1;
    }
    char *kbuf = (char*)malloc(count);
    copyin(p->pgd, kbuf, (uintptr_t)buf, count);
    ssize_t ret = write(p->fd_table[fd], kbuf, count);
    free(kbuf);
    return ret;
}

int sys_creat(struct trap_frame *ctx)
{
    char path[128];
    char *s = path;
    char *d = (char*)ctx->a0;
    struct proc *p = current_proc();
    for(;;)
    {
        copyin(p->pgd, s, (uintptr_t)d, 1);
        if(*s == '\0' || s - path >= 127)
        {
            break;
        }
        s++;
        d++;
    }
    int mode = (int)ctx->a1;
    struct dentry *dentry = mkdir(path, mode);
    if(dentry == NULL || dentry->d_inode == NULL)
    {
        return -1;
    }
    struct file *file = open(path, 0);
    return alloc_fd(p, file);
}

int sys_mkdir(struct trap_frame *ctx)
{
    char path[128];
    char *s = path;
    char *d = (char*)ctx->a0;
    struct proc *p = current_proc();
    for(;;)
    {
        copyin(p->pgd, s, (uintptr_t)d, 1);
        if(*s == '\0' || s - path >= 127)
        {
            break;
        }
        s++;
        d++;
    }
    int mode = ctx->a1;
    struct dentry *dentry = mkdir(path, mode);
    if(dentry == NULL || dentry->d_inode == NULL)
    {
        return -1;
    }
    return 0;
}

typedef int (*sysfuncPtr)(struct trap_frame *ctx); // 定义函数指针类型 FuncPtr

static sysfuncPtr syscalls[] = {
    [SYSCALL_PRINT] = sys_printf,
    [SYSCALL_YIELD] = sys_yield,
    [SYSCALL_OPEN]  = sys_open,
    [SYSCALL_CLOSE] = sys_close,
    [SYSCALL_READ]  = sys_read,
    [SYSCALL_WRITE] = sys_write,
    [SYSCALL_CREAT] = sys_creat,
    [SYSCALL_MKDIR] = sys_mkdir,
};

void do_syscall(struct trap_frame *ctx)
{
    uint64_t num = ctx->a7; // 获取系统调用号
    if (num < SYSCALL_END)
    {
        ctx->a0 = syscalls[num](ctx); // 执行对应的处理函数，并将返回值存入 a0 中
    }
    else
    {
        // printk("syscall %l not implemented\n", num);
        ctx->a0 = -1; // 如果系统调用号超出范围，则返回错误码 -1
    }
}
