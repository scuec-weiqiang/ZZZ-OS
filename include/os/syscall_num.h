#ifndef SYSCALL_NUM_H
#define SYSCALL_NUM_H

#include <asm/ptrace.h>

/*
 * 单一数据源：系统调用号、函数声明、派发表均由此生成。
 * 新增系统调用只需在此加一行。
 * 格式: X(编号, 函数名)
 */
#define SYSCALL_LIST \
    X(17, getcwd) \
    X(23, dup) \
    X(24, dup3) \
    X(34, mkdirat) \
    X(35, unlinkat) \
    X(48, faccessat) \
    X(49, chdir) \
    X(56, openat) \
    X(57, close) \
    X(59, pipe2) \
    X(61, getdents) \
    X(62, lseek) \
    X(63, read) \
    X(64, write) \
    X(79, newfstatat) \
    X(80, fstat) \
    X(93, exit) \
    X(94, exit_group) \
    X(113, clock_gettime) \
    X(114, clock_getres) \
    X(129, kill) \
    X(134, sigaction) \
    X(139, sigreturn) \
    X(172, getpid) \
    X(214, brk) \
    X(215, munmap) \
    X(220, clone) \
    X(221, execve) \
    X(222, mmap) \
    X(226, mprotect) \
    X(260, wait4) \
    X(201, ps) \

/* 生成系统调用号枚举 */
#define X(nr, name) SYSCALL_##name = nr,
enum {
    SYSCALL_LIST
};
#undef X

#define SYSCALL_MAX 512  /* 派发表大小，需大于最大 syscall 编号 */

/* 统一函数签名：从 pt_regs 中取参数，返回值写入 r[0] */
typedef long (*syscall_fn_t)(struct pt_regs *ctx);

/* 生成函数原型声明 */
#define X(nr, name) long sys_##name(struct pt_regs *ctx);
SYSCALL_LIST
#undef X

/* 派发表，内核使用 */
extern const syscall_fn_t syscall_table[SYSCALL_MAX];

#endif
