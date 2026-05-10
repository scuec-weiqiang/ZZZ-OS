#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

void _init(void) {}
void _fini(void) {}

extern int __syscall0(int nr);
extern int __syscall1(int nr, int a0);
extern int __syscall2(int nr, int a0, int a1);
extern int __syscall3(int nr, int a0, int a1, int a2);

#define SYS_exit   1
#define SYS_fork   2
#define SYS_read   3
#define SYS_write  4
#define SYS_open   5
#define SYS_close  6
#define SYS_fstat  7
#define SYS_stat   8
#define SYS_kill   9
#define SYS_chdir  12
#define SYS_lseek  19
#define SYS_getpid 20
#define SYS_pipe   22
#define SYS_yield  24
#define SYS_brk    45
#define SYS_execve 59
#define SYS_waitpid 106
#define SYS_getdents 141
#define SYS_getcwd 183

extern char _end;
static char *heap_end;


void *_sbrk(ptrdiff_t incr) {
    if (!heap_end)
        heap_end = &_end;

    char *prev = heap_end;
    char *next = heap_end + incr;

    long ret = __syscall1(SYS_brk, (long)next);
    if (ret < 0) {
        errno = -ret;
        return (void *)-1;
    }

    heap_end = next;
    return prev;
}

int _fork(void) {
    long ret = __syscall0(SYS_fork);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _write(int fd, const void *buf, size_t count) {
    long ret = __syscall3(SYS_write, fd, (long)buf, count);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _open(const char *path, int flags, int mode) {
    long ret = __syscall3(SYS_open, (long)path, flags, mode);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _read(int fd, void *buf, size_t count) {
    long ret = __syscall3(SYS_read, fd, (long)buf, count);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _close(int fd) {
    long ret = __syscall1(SYS_close, fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _chdir(const char *path) {
    long ret = __syscall1(SYS_chdir, (long)path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int chdir(const char *path) {
    return _chdir(path);
}

char *_getcwd(char *buf, size_t size) {
    long ret;

    if (buf == NULL || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    ret = __syscall2(SYS_getcwd, (long)buf, size);
    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    return buf;
}

char *getcwd(char *buf, size_t size) {
    return _getcwd(buf, size);
}

int _lseek(int fd, int offset, int whence) {
    long ret = __syscall3(SYS_lseek, fd, offset, whence);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _fstat(int fd, struct stat *st) {
    long ret = __syscall2(SYS_fstat, fd, (long)st);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _stat(const char *path, struct stat *st) {
    long ret = __syscall2(SYS_stat, (long)path, (long)st);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _isatty(int fd) {
    return fd >= 0 && fd <= 2;
}

int _getpid(void) {
    long ret = __syscall1(SYS_getpid, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

void _exit(int status) {
    __syscall1(SYS_exit, status);
    while (1) {}
}

int _wait(int *status) {
    long ret = __syscall3(SYS_waitpid, -1, (long)status, 0);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _execve(const char *filename, char *const argv[], char *const envp[]) {
    __syscall3(SYS_execve, (long)filename, (long)argv, (long)envp);
    return -1; // execve 成功不会返回，失败才会返回 -1
}

int _getdents(int fd, void *dirp, unsigned int count) {
    long ret = __syscall3(SYS_getdents, fd, (long)dirp, count);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _kill(int pid, int sig) {
    long ret = __syscall2(SYS_kill, pid, sig);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int _pipe(int pipefd[2]) {
    long ret = __syscall1(SYS_pipe, (long)pipefd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}
