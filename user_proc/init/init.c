#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
    static const char entered[] = "init: entered user mode\n";
    int fd;
    pid_t self;

    (void)argc;
    (void)argv;

    write(1, entered, sizeof(entered) - 1);

    self = getpid();
    if (getsid(0) != self && setsid() < 0) {
        printf("init: setsid failed\n");
        return 1;
    }

    fd = open("/dev/ttyS0", O_RDWR);
    if (fd < 0)
        return 1;
    if (ioctl(fd, TIOCSCTTY, 0) < 0)
        return 1;
    if (dup2(fd, 0) < 0 || dup2(fd, 1) < 0 || dup2(fd, 2) < 0)
        return 1;

    printf("init process started, pid=%d\n", self);
   
    pid_t pid = fork();

    printf("after fork, pid=%d\n", pid);

    char *child_argv[] = { "/bin/dash", NULL };

    char *envp[] = {
    "PATH=/bin",
    "HOME=/",
    "TERM=vt100",
    "PS1=wqqqq@ZZZ:$PWD$ ",
    NULL
};
    if (pid == 0) {
        printf("now pid = %d\n",getpid());
        execve("/bin/dash", child_argv, envp);
    } else {
        printf("now pid = %d\n",getpid());
        wait(NULL);
    }
    return 0;
}
