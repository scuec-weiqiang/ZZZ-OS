#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SYS_getcwd 17
#define SYS_chdir 49
#define SYS_close 57
#define SYS_pipe2 59
#define SYS_exit 93
#define SYS_clone 220
#define SYS_execve 221
#define SYS_wait4 260

#define EBADF 9
#define SIGCHLD 17

#define CLONE_VM 0x00000100UL
#define CLONE_FS 0x00000200UL
#define CLONE_FILES 0x00000400UL
#define CLONE_VFORK 0x00004000UL

#define STACK_SIZE 8192
#define EXEC_MAGIC 0x66778899
#define VFORK_DONE 0x55667788

enum child_mode {
    CHILD_NONE,
    CHILD_CLOSE_FD,
    CHILD_CHDIR_BIN,
    CHILD_VFORK_MARK,
    CHILD_EXEC_TRUE,
};

static unsigned char child_stack[STACK_SIZE] __attribute__((aligned(16)));
static volatile int child_mode;
static volatile int child_fd;
static volatile int shared_mark;

static char bin_path[] = "/bin";
static char true_path[] = "/bin/true";
static char *true_argv[] = { true_path, NULL };
static char *empty_envp[] = { NULL };

static long raw_syscall1(long nr, long arg0)
{
    register long a0 asm("a0") = arg0;
    register long a7 asm("a7") = nr;

    asm volatile("ecall" : "+r"(a0) : "r"(a7) : "memory");
    return a0;
}

static long raw_syscall2(long nr, long arg0, long arg1)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a7 asm("a7") = nr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a7)
                 : "memory");
    return a0;
}

static long raw_syscall3(long nr, long arg0, long arg1, long arg2)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a7 asm("a7") = nr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a7)
                 : "memory");
    return a0;
}

static long raw_syscall4(long nr, long arg0, long arg1, long arg2, long arg3)
{
    register long a0 asm("a0") = arg0;
    register long a1 asm("a1") = arg1;
    register long a2 asm("a2") = arg2;
    register long a3 asm("a3") = arg3;
    register long a7 asm("a7") = nr;

    asm volatile("ecall"
                 : "+r"(a0)
                 : "r"(a1), "r"(a2), "r"(a3), "r"(a7)
                 : "memory");
    return a0;
}

__attribute__((noreturn, noinline)) void clone_flags_child_entry(void)
{
    if (child_mode == CHILD_CLOSE_FD) {
        raw_syscall1(SYS_close, child_fd);
        raw_syscall1(SYS_exit, 0);
    }

    if (child_mode == CHILD_CHDIR_BIN) {
        raw_syscall1(SYS_chdir, (long)bin_path);
        raw_syscall1(SYS_exit, 0);
    }

    if (child_mode == CHILD_VFORK_MARK) {
        for (volatile int i = 0; i < 1000000; i++) {
        }
        shared_mark = VFORK_DONE;
        raw_syscall1(SYS_exit, 0);
    }

    if (child_mode == CHILD_EXEC_TRUE) {
        raw_syscall3(SYS_execve, (long)true_path, (long)true_argv,
                     (long)empty_envp);
        raw_syscall1(SYS_exit, 127);
    }

    raw_syscall1(SYS_exit, 125);
    for (;;) {
    }
}

static long clone_child(unsigned long flags)
{
    unsigned long stack_top;
    register long a0 asm("a0");
    register long a1 asm("a1");
    register long a2 asm("a2");
    register long a3 asm("a3");
    register long a4 asm("a4");
    register long a7 asm("a7");

    stack_top = (unsigned long)child_stack + sizeof(child_stack);
    stack_top &= ~0xfUL;

    a0 = flags;
    a1 = stack_top;
    a2 = 0;
    a3 = 0;
    a4 = 0;
    a7 = SYS_clone;

    asm volatile(
        "ecall\n"
        "bnez a0, 1f\n"
        "call clone_flags_child_entry\n"
        "1:\n"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a7)
        : "memory");

    return a0;
}

static int wait_child(long pid)
{
    int status = 0;
    long ret;

    ret = raw_syscall4(SYS_wait4, pid, (long)&status, 0, 0);
    if (ret != pid) {
        printf("FAIL: wait4 ret=%ld expected=%ld\n", ret, pid);
        return 1;
    }

    return 0;
}

static int test_clone_files(void)
{
    int fds[2] = { -1, -1 };
    long pid;
    long ret;
    int fails = 0;

    ret = raw_syscall2(SYS_pipe2, (long)fds, 0);
    if (ret < 0) {
        printf("FAIL: pipe2 ret=%ld\n", ret);
        return 1;
    }

    child_mode = CHILD_CLOSE_FD;
    child_fd = fds[0];
    pid = clone_child(CLONE_FILES | SIGCHLD);
    if (pid < 0) {
        printf("FAIL: CLONE_FILES clone ret=%ld\n", pid);
        raw_syscall1(SYS_close, fds[0]);
        raw_syscall1(SYS_close, fds[1]);
        return 1;
    }

    fails += wait_child(pid);

    ret = raw_syscall1(SYS_close, fds[0]);
    if (ret != -EBADF) {
        printf("FAIL: CLONE_FILES close ret=%ld expected=%d\n", ret, -EBADF);
        fails++;
    } else {
        printf("ok: CLONE_FILES shared close\n");
    }

    raw_syscall1(SYS_close, fds[1]);
    return fails;
}

static int test_clone_fs(void)
{
    char cwd[64];
    long pid;
    long ret;
    int fails = 0;

    child_mode = CHILD_CHDIR_BIN;
    pid = clone_child(CLONE_FS | SIGCHLD);
    if (pid < 0) {
        printf("FAIL: CLONE_FS clone ret=%ld\n", pid);
        return 1;
    }

    fails += wait_child(pid);

    memset(cwd, 0, sizeof(cwd));
    ret = raw_syscall2(SYS_getcwd, (long)cwd, sizeof(cwd));
    if (ret < 0 || strcmp(cwd, "/bin") != 0) {
        printf("FAIL: CLONE_FS cwd ret=%ld cwd='%s'\n", ret, cwd);
        fails++;
    } else {
        printf("ok: CLONE_FS shared cwd\n");
    }

    raw_syscall1(SYS_chdir, (long)"/");
    return fails;
}

static int test_clone_vfork(void)
{
    long pid;
    int fails = 0;

    shared_mark = 0;
    child_mode = CHILD_VFORK_MARK;
    pid = clone_child(CLONE_VM | CLONE_VFORK | SIGCHLD);
    if (pid < 0) {
        printf("FAIL: CLONE_VFORK clone ret=%ld\n", pid);
        return 1;
    }

    if (shared_mark != VFORK_DONE) {
        printf("FAIL: CLONE_VFORK returned before child exit mark=0x%x\n",
               shared_mark);
        fails++;
    } else {
        printf("ok: CLONE_VFORK parent waited for child\n");
    }

    fails += wait_child(pid);
    return fails;
}

static int test_clone_vm_execve(void)
{
    long pid;
    int fails = 0;

    shared_mark = EXEC_MAGIC;
    child_mode = CHILD_EXEC_TRUE;
    pid = clone_child(CLONE_VM | SIGCHLD);
    if (pid < 0) {
        printf("FAIL: CLONE_VM execve clone ret=%ld\n", pid);
        return 1;
    }

    fails += wait_child(pid);

    if (shared_mark != EXEC_MAGIC) {
        printf("FAIL: parent mm corrupted after child execve mark=0x%x\n",
               shared_mark);
        fails++;
    } else {
        printf("ok: CLONE_VM child execve kept parent mm alive\n");
    }

    return fails;
}

int main(void)
{
    int fails = 0;

    printf("clone-flags-test\n");
    fails += test_clone_files();
    fails += test_clone_fs();
    fails += test_clone_vfork();
    fails += test_clone_vm_execve();

    if (fails == 0) {
        printf("PASS\n");
        return 0;
    }

    printf("FAIL: %d checks failed\n", fails);
    return 1;
}
