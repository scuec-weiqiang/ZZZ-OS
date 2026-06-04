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

    pid_t pid = fork();

    printf("after fork, pid=%d\n", pid);

    char *child_argv[] = { "/bin/ls", NULL };
    if (pid == 0) {
        printf("now pid = %d\n",getpid());
        execve("/bin/ls", child_argv,NULL);
    } else {
        printf("now pid = %d\n",getpid());
        wait(NULL);
    }
    return 0;
}
