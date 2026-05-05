#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>

char led_on[] = "1\n";
char led_off[] = "0\n";

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
  
    int fd = open("/dev/myledcdev",O_RDWR);
    write(fd, led_on,sizeof(led_on));
    close (fd);

    printf("hello! pid=%d, led on\n", getpid());

    fd = open("/hello.txt", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    char buf[128];
    ssize_t bytes_read ;
    bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read < 0) {
        perror("read");
        close(fd);
        return 1;
    }
    buf[bytes_read] = '\0'; // Null-terminate the buffer
    printf("Read from /hello.txt: %s\n", buf);
    
    const char *msg = "Hello, ext2 file!\n";
    ssize_t bytes_written = write(fd, msg, strlen(msg));
    if (bytes_written < 0) {
        perror("write");
        close(fd);
        return 1;
    }
    printf("Wrote to /hello.txt: %s\n", msg);

    lseek(fd, 0, SEEK_SET);

    bytes_read = read(fd, buf, sizeof(buf) - 1);
    if (bytes_read <= 0) {
        printf("No more data to read from /hello.txt\n");
        close(fd);
        return 1;
    }

    buf[bytes_read] = '\0'; // Null-terminate the buffer
    printf("Read from /hello.txt: %s\n", buf);

    close(fd);

    mkdir("/my", 0644);
    return 0;
}
