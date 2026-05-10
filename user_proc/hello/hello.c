#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <sched.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>

char led_on[] = "1\n";
char led_off[] = "0\n";

void print_stat(const struct stat *st) {
    printf("=== struct stat 信息 ===\n");
    printf("设备编号:    %lu\n", (unsigned long)st->st_dev);
    printf("inode 编号:  %lu\n", (unsigned long)st->st_ino);
    printf("权限/类型:   %u\n", (unsigned int)st->st_mode);
    printf("硬链接数:    %lu\n", (unsigned long)st->st_nlink);
    printf("UID:         %u\n", (unsigned int)st->st_uid);
    printf("GID:         %u\n", (unsigned int)st->st_gid);
    printf("设备类型:    %lu\n", (unsigned long)st->st_rdev);
    printf("文件大小:    %ld 字节\n", (long)st->st_size);
    printf("块大小:      %ld\n", (long)st->st_blksize);
    printf("块数:        %ld\n", (long)st->st_blocks);
    printf("访问时间:    %ld\n", (long)st->st_atime);
    printf("修改时间:    %ld\n", (long)st->st_mtime);
    printf("创建时间:    %ld\n", (long)st->st_ctime);
    printf("=======================\n");
}

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

    memset(buf, 0, 128);
    if (fstat(fd, (struct stat *)buf)) {
        perror("fstat");
        close(fd);
        return 1;
    }
    print_stat((struct stat *)buf);
    
    close(fd);

    kill(getpid(), SIGTERM);

    return 0;
}
