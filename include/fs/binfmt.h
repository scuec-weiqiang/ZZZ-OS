#ifndef _LINUX_BINFMTS_H
#define _LINUX_BINFMTS_H

#include <os/types.h>
#include <os/pfn.h>
#include <os/list.h>

#define BINPRM_BUF_SIZE 128
#define MAX_ARG_STRLEN (PAGE_SIZE * 32)
#define MAX_ARG_STRINGS 0x7FFFFFFF

#if SYS_BITS == 64
#define USER_STACK_TOP   0x0000003f00000000UL
#else
#define USER_STACK_TOP   0xbf000000UL
#endif
#define USER_STACK_PAGES 4
#define USER_STACK_SIZE  (USER_STACK_PAGES * PAGE_SIZE)
#define USER_SIGTRAMP_ADDR (USER_STACK_TOP - USER_STACK_SIZE - PAGE_SIZE)
#if SYS_BITS == 64
#define USER_STACK_ALIGN 16UL
#else
#define USER_STACK_ALIGN 8UL
#endif

struct mm_struct;
struct file;

/*
 * 简化版 linux_binprm：exec 过程中的参数载体
 * 从 Linux fs/exec.c 的 struct linux_binprm 简化而来
 */
struct linux_binprm {
    char buf[BINPRM_BUF_SIZE];        /* 文件头缓冲（至少128字节） */
    struct mm_struct *mm;             /* 新进程的 mm */
    unsigned long p;                  /* 当前栈顶位置（argv/envp 存放处） */
    int argc, envc;                   /* 参数和环境变量个数 */
    const char *filename;             /* 可执行文件路径 */
    const char *interp;               /* 实际执行的二进制路径（通常同 filename） */
    struct file *file;                /* 已打开的可执行文件 */
    unsigned long arg_start;
    unsigned long env_start;
};

/*
 * 简化版 linux_binfmt：二进制格式处理器
 * 从 Linux include/linux/binfmts.h 的 struct linux_binfmt 简化而来
 */
struct linux_binfmt {
    struct list_head lh;              /* 全局 binfmt 链表 */
    int (*load_binary)(struct linux_binprm *bprm);
};

extern int register_binfmt(struct linux_binfmt *fmt);
extern int unregister_binfmt(struct linux_binfmt *fmt);
extern int search_binary_handler(struct linux_binprm *bprm);
extern void set_binfmt(struct linux_binfmt *new);
extern int flush_old_exec(struct linux_binprm *bprm);
extern void elf_binfmt_init(void);

#endif /* _LINUX_BINFMTS_H */
