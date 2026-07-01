#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/ps.h>

void _init(void) {}
void _fini(void) {}

extern long __syscall0(long nr);
extern long __syscall1(long nr, long a0);
extern long __syscall2(long nr, long a0, long a1);
extern long __syscall3(long nr, long a0, long a1, long a2);
extern long __syscall6(long nr, long a0, long a1, long a2, long a3, long a4, long a5);

#define SYS_exit   1
#define SYS_fork   2
#define SYS_read   3
#define SYS_write  4
#define SYS_open   5
#define SYS_close  6
#define SYS_fstat  7
#define SYS_stat   8
#define SYS_kill   9
#define SYS_dup    10
#define SYS_dup2   11
#define SYS_chdir  12
#define SYS_sigreturn 13
#define sys_sigaction 14
#define SYS_lseek  19
#define SYS_getpid 20
#define SYS_pipe   22
#define SYS_access 33
#define SYS_rmdir  40
#define SYS_unlink 41
#define SYS_yield  24
#define SYS_brk    45
#define SYS_creat  46
#define SYS_mkdir  47
#define SYS_execve 59
#define SYS_munmap 91
#define SYS_waitpid 106
#define SYS_mprotect 125
#define SYS_getdents 141
#define SYS_getcwd 183
#define SYS_mmap   192
#define SYS_ps     201

extern char _end;
extern char **environ;
static char *heap_end;

#define EXECVP_PATH_MAX 256


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
    // printf("_read fd=%d len=%d\n", fd, count);
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

int chdir(const char *path) {
    long ret = __syscall1(SYS_chdir, (long)path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int access(const char *path, int mode) {
    long ret = __syscall2(SYS_access, (long)path, mode);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int unlink(const char *path) {
    long ret = __syscall1(SYS_unlink, (long)path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int rmdir(const char *path) {
    long ret = __syscall1(SYS_rmdir, (long)path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
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

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    long ret = __syscall6(SYS_mmap, (long)addr, length, prot, flags, fd, offset);
    if (ret < 0) {
        errno = -ret;
        return MAP_FAILED;
    }
    return (void *)ret;
}

int munmap(void *addr, size_t length) {
    long ret = __syscall2(SYS_munmap, (long)addr, length);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int mprotect(void *addr, size_t length, int prot) {
    long ret = __syscall3(SYS_mprotect, (long)addr, length, prot);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
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

int creat(const char *path, mode_t mode) {
    long ret = __syscall2(SYS_creat, (long)path, mode);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int mkdir(const char *path, mode_t mode) {
    long ret = __syscall2(SYS_mkdir, (long)path, mode);
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

int _ps(struct ps_info *buf, int max) {
    long ret = __syscall2(SYS_ps, (long)buf, max);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _getpgrp(void) {
    return _getpid();
}

int _setpgid(pid_t pid, pid_t pgid) {
    return 0;
}

int _tcgetpgrp(int fd) {
    return _getpid();
}

int _tcsetpgrp(int fd, pid_t pgrp) {
    return 0;
}

int _tcgetattr(int fd, void *termios_p) {
    return 0;
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

int waitpid(pid_t pid, int *status, int options) {
    long ret = __syscall3(SYS_waitpid, pid, (long)status, options);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int _execve(const char *filename, char *const argv[], char *const envp[]) {
    long ret = __syscall3(SYS_execve, (long)filename, (long)argv, (long)envp);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret; // execve 成功不会返回
}

static int execvp_try_path(const char *dir, size_t dir_len,
                           const char *file, char *const argv[])
{
    char path[EXECVP_PATH_MAX];
    size_t file_len;
    size_t pos = 0;

    if (dir_len == 0) {
        return _execve(file, argv, environ);
    }

    file_len = strlen(file);
    if (dir_len + file_len + 2 > sizeof(path)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    memcpy(path, dir, dir_len);
    pos = dir_len;
    if (pos > 0 && path[pos - 1] != '/') {
        path[pos++] = '/';
    }

    memcpy(path + pos, file, file_len);
    pos += file_len;
    path[pos] = '\0';

    return _execve(path, argv, environ);
}

int execvp(const char *file, char *const argv[]) {
    const char *path_env;
    const char *seg;
    int last_errno = ENOENT;

    if (file == NULL || file[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    if (strchr(file, '/') != NULL) {
        return _execve(file, argv, environ);
    }

    path_env = getenv("PATH");
    if (path_env == NULL || path_env[0] == '\0') {
        path_env = "/bin";
    }

    seg = path_env;
    while (1) {
        const char *end = strchr(seg, ':');
        size_t seg_len = end ? (size_t)(end - seg) : strlen(seg);

        if (execvp_try_path(seg, seg_len, file, argv) >= 0) {
            return 0;
        }

        if (errno != ENOENT && errno != ENOTDIR) {
            return -1;
        }
        last_errno = errno;

        if (end == NULL) {
            break;
        }
        seg = end + 1;
    }

    errno = last_errno;
    return -1;
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

int pipe(int pipefd[2]) {
    long ret = __syscall1(SYS_pipe, (long)pipefd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int dup(int oldfd) {
    long ret = __syscall1(SYS_dup, oldfd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int dup2(int oldfd, int newfd) {
    long ret = __syscall2(SYS_dup2, oldfd, newfd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int sigreturn(void) {
    long ret = __syscall0(SYS_sigreturn);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

struct sigaction {
    void (*handler)(int );
    unsigned long mask;
    int flags;
};

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    long ret = __syscall3(sys_sigaction, signum, (long)act, (long)oldact);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}
