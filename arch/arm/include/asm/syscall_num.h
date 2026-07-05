#ifndef __ASM_ARM_SYSCALL_NUM_H
#define __ASM_ARM_SYSCALL_NUM_H

#include <asm/ptrace.h>

/*
 * ARM EABI Linux syscall ABI numbers for the syscall names currently wired
 * through the common syscall table.
 */
#define SYSCALL_LIST \
    X(1, exit) \
    X(3, read) \
    X(4, write) \
    X(6, close) \
    X(11, execve) \
    X(12, chdir) \
    X(19, lseek) \
    X(20, getpid) \
    X(37, kill) \
    X(41, dup) \
    X(45, brk) \
    X(54, ioctl) \
    X(91, munmap) \
    X(108, fstat) \
    X(114, wait4) \
    X(120, clone) \
    X(125, mprotect) \
    X(173, sigreturn) \
    X(174, sigaction) \
    X(175, rt_sigprocmask) \
    X(183, getcwd) \
    X(192, mmap) \
    X(217, getdents) \
    X(248, exit_group) \
    X(256, set_tid_address) \
    X(263, clock_gettime) \
    X(264, clock_getres) \
    X(281, socket) \
    X(322, openat) \
    X(323, mkdirat) \
    X(327, newfstatat) \
    X(328, unlinkat) \
    X(331, symlinkat) \
    X(332, readlinkat) \
    X(334, faccessat) \
    X(338, set_robust_list) \
    X(358, dup3) \
    X(359, pipe2) \
    X(369, prlimit64) \
    X(382, renameat2) \
    X(384, getrandom) \
    X(398, rseq) \
    X(201, ps) \

#define X(nr, name) SYSCALL_##name = nr,
enum {
    SYSCALL_LIST
};
#undef X

#define SYSCALL_MAX 512

typedef long (*syscall_fn_t)(struct pt_regs *ctx);

#define X(nr, name) long sys_##name(struct pt_regs *ctx);
SYSCALL_LIST
#undef X

extern const syscall_fn_t syscall_table[SYSCALL_MAX];

#endif
