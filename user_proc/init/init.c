#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("init process started, pid=%d\n", getpid());

    setsid();
    int fd = open("/dev/ttyS0", O_RDWR);
    ioctl(fd, TIOCSCTTY, 0);
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
   
    pid_t pid = fork();

    printf("after fork, pid=%d\n", pid);

    char *child_argv[] = { "/bin/simple-c-shell", NULL };

    char *envp[] = {
    "PATH=/bin",
    "HOME=/",
    "TERM=vt100",
    "PS1=$ ",
    NULL
};
    if (pid == 0) {
        printf("now pid = %d\n",getpid());
        execve("/bin/simple-c-shell", child_argv, envp);
    } else {
        printf("now pid = %d\n",getpid());
        wait(NULL);
    }
    return 0;
}
