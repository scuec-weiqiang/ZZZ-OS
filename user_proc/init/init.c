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

    char *child_argv[] = { "/bin/simple-c-shell", NULL };
    if (pid == 0) {
        printf("now pid = %d\n",getpid());
        execve("/bin/simple-c-shell", child_argv,NULL);
    } else {
        printf("now pid = %d\n",getpid());
        wait(NULL);
    }
    

    // char *child_argv[] = { "/bin/hello", NULL };
    // if (pid == 0) {
    //     execve("/bin/hello", child_argv,NULL);
    // } else {
    //     wait(NULL);
    // }
    return 0;
}
