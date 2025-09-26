/**
 * @FilePath: /ZZZ/kernel/syscall.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-02 18:11:27
 * @LastEditTime: 2025-05-02 18:59:49
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "syscall.h"
#include "syscall_num.h"

#include "riscv.h"
#include "printk.h"
#include "proc.h"
#include "sched.h"
#include "vfs.h"

int sys_yield(struct trap_frame *ctx)
{
    // printk("sys_yield called by %xu\n", ctx->a0);
    yield(); // 调用调度函数，切换到其他线程
    return 0;
}

int sys_open(struct trap_frame *ctx)
{
    char *path = (char*)((uintptr_t)ctx->a0 - 0x10000 + (uintptr_t)scheduler[0].current->code[0]); // 获取文件路径指针
    int flags = ctx->a1;
    struct file *file = open(path, flags);
    return (int)file;
}

int sys_printf(struct trap_frame *ctx)
{
    char *s = malloc(ctx->a1); // 获取字符串指针
    copyin(scheduler[0].current->pgd, s, (uintptr_t)ctx->a0, (uintptr_t)ctx->a1);
    int n = printk("%s", s);
    free(s);
    return n;
}

typedef int (*sysfuncPtr)(struct trap_frame *ctx); // 定义函数指针类型 FuncPtr

static sysfuncPtr syscalls[] = {
    [SYSCALL_PRINTF] = sys_printf,
    [SYSCALL_YIELD] = sys_yield,
};

void do_syscall(struct trap_frame *ctx)
{
    u64 num = ctx->a7; // 获取系统调用号
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
