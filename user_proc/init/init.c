#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("init process started, pid=%d\n", getpid());

    int fd = open("/dev/console", O_RDWR);
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2) close(fd);

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
