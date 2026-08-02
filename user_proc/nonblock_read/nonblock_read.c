/**
 * @FilePath     : /ZZZ-OS/user_proc/nonblock_read/nonblock_read.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-24 17:35:08
 * @LastEditTime : 2026-07-24 17:49:21
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <errno.h>

static struct termios oldt;

int tty_set_noncanonical(int fd)
{
    struct termios t;
    int flags;

    if (tcgetattr(fd, &oldt) < 0)
        return -1;

    t = oldt;

    t.c_lflag &= ~(ICANON | ECHO);

    /*
     * VMIN=0, VTIME=0:
     * read() 立即返回。
     * 有数据返回字节数，没数据返回 0。
     *
     * 如果 fd 同时设置了 O_NONBLOCK，
     * 没数据时通常返回 -1/EAGAIN。
     */
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &t) < 0)
        return -1;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;

    return 0;
}

void tty_restore(int fd)
{
    tcsetattr(fd, TCSANOW, &oldt);
}

int main(void)
{
    int fd;
    char buf[128];

    fd = open("/dev/ttyS0", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        printf("open failed: errno=%d, %s\n", errno, strerror(errno));
        return 1;
    }

    tty_set_noncanonical(fd);

    printf("nonblocking tty read test started\n");

    for (;;) {
        ssize_t n = read(fd, buf, sizeof(buf));

        if (n > 0) {
            printf("read %ld bytes: ", (long)n);
            write(STDOUT_FILENO, buf, (size_t)n);
            printf("\n");
        } else if (n == 0) {
            /*
             * 普通TTY没有数据时，非阻塞read不应该返回0，
             * 一般应该返回-1，并设置errno=EAGAIN。
             *
             * 在规范模式下，Ctrl+D可能让read返回0。
             */
            printf("read returned EOF\n");
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // printf("no data, EAGAIN\n");
            } else if (errno == EINTR) {
                printf("read interrupted\n");
            } else {
                printf("read failed: errno=%d, %s\n",
                       errno, strerror(errno));
                break;
            }
        }

        /*
         * 防止疯狂循环刷屏。
         * 如果你的系统暂时没有sleep/usleep，可以替换成简单忙等待。
         */
        // sleep(1);
    }

    close(fd);
    return 0;
}