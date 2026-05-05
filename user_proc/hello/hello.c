#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>

char led_on[] = "1\n";
char led_off[] = "0\n";

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
  
    int fd = open("/dev/myledcdev",O_RDWR);
    write(fd, led_on,sizeof(led_on));
    
    printf("hell0! pid=%d, led on\n", getpid());

    return 0;
}
