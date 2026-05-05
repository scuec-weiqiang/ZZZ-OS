#ifndef SYSCALL_NUM_H
#define SYSCALL_NUM_H

#include <asm/ptrace.h>

/*
 * 单一数据源：系统调用号、函数声明、派发表均由此生成。
 * 新增系统调用只需在此加一行。
 * 格式: X(编号, 函数名)
 */
#define SYSCALL_LIST \
    X(1,  exit)    \
    X(2,  fork)    \
    X(3,  read)    \
    X(4,  write)   \
    X(5,  open)    \
    X(6,  close)   \
    X(19, lseek)   \
    X(20, getpid)  \
    X(45, brk)     \
    X(46, creat)   \
    X(47, mkdir)  \
    X(59, execve)  \
    X(106, waitpid) \
    X(141, getdents) \

/* 生成系统调用号枚举 */
#define X(nr, name) SYSCALL_##name = nr,
enum {
    SYSCALL_LIST
};
#undef X

#define SYSCALL_MAX 256  /* 派发表大小，需大于最大 syscall 编号 */

/* 统一函数签名：从 pt_regs 中取参数，返回值写入 r[0] */
typedef long (*syscall_fn_t)(struct pt_regs *ctx);

/* 生成函数原型声明 */
#define X(nr, name) long sys_##name(struct pt_regs *ctx);
SYSCALL_LIST
#undef X

/* 派发表，内核使用 */
extern const syscall_fn_t syscall_table[SYSCALL_MAX];

#endif
