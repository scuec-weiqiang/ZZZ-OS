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
    // char line[128];
    printf("hell0! pid=%d\n", getpid());
          
    // while (1) {
    //     if (fgets(line, sizeof(line), stdin) == NULL) {
    //         printf("\n");
    //         break;
    //     }
    // }
    return 0;
}
