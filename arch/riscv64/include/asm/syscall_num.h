#ifndef __ASM_RISCV64_SYSCALL_NUM_H
#define __ASM_RISCV64_SYSCALL_NUM_H

#include <asm/ptrace.h>

/*
 * RISC-V Linux syscall ABI numbers.
 * Format: X(number, syscall_name)
 */
#define SYSCALL_LIST \
    X(17, getcwd) \
    X(23, dup) \
    X(24, dup3) \
    X(25, fcntl) \
    X(29, ioctl) \
    X(34, mkdirat) \
    X(35, unlinkat) \
    X(36, symlinkat) \
    X(48, faccessat) \
    X(49, chdir) \
    X(53, fchmodat) \
    X(56, openat) \
    X(57, close) \
    X(59, pipe2) \
    X(61, getdents) \
    X(62, lseek) \
    X(63, read) \
    X(64, write) \
    X(77, tee) \
    X(78, readlinkat) \
    X(79, newfstatat) \
    X(80, fstat) \
    X(88, utimensat) \
    X(93, exit) \
    X(94, exit_group) \
    X(96, set_tid_address) \
    X(99, set_robust_list) \
    X(113, clock_gettime) \
    X(114, clock_getres) \
    X(129, kill) \
    X(134, sigaction) \
    X(135, rt_sigprocmask) \
    X(139, sigreturn) \
    X(154, setpgid) \
    X(155, getpgid) \
    X(156, getsid) \
    X(157, setsid) \
    X(166, umask) \
    X(172, getpid) \
    X(173, getppid) \
    X(174, getuid) \
    X(175, geteuid) \
    X(198, socket) \
    X(201, ps) \
    X(214, brk) \
    X(215, munmap) \
    X(220, clone) \
    X(221, execve) \
    X(222, mmap) \
    X(226, mprotect) \
    X(258, riscv_hwprobe) \
    X(260, wait4) \
    X(261, prlimit64) \
    X(276, renameat2) \
    X(278, getrandom) \
    X(293, rseq) \
    X(439, faccessat2) \

#define X(nr, name) SYSCALL_##name = nr,
enum {
    SYSCALL_LIST
};
#undef X

#ifdef SYS_TRACE_ENABLE
    #define X(nr, name) [nr] = #name,

    static char *syscall_names[] = {
        SYSCALL_LIST
    };
    #undef X
#endif

#define SYSCALL_MAX 512

typedef long (*syscall_fn_t)(struct pt_regs *ctx);

#define X(nr, name) long sys_##name(struct pt_regs *ctx);
SYSCALL_LIST
#undef X

extern const syscall_fn_t syscall_table[SYSCALL_MAX];

#endif
