#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define FB_PATH "/dev/fb0"
#define TTY_PATH "/dev/ttyS0"

#define FB_WIDTH  640
#define FB_HEIGHT 400
#define FB_BPP    32
#define FB_BYTES_PER_PIXEL (FB_BPP / 8)
#define FB_SIZE ((size_t)FB_WIDTH * FB_HEIGHT * FB_BYTES_PER_PIXEL)

#define PLAYER_W 48
#define PLAYER_H 48
#define PLAYER_STEP 12

static struct termios old_termios;
static int old_termios_valid;
static int old_tty_flags;
static int old_tty_flags_valid;

static uint32_t rgb(unsigned int r, unsigned int g, unsigned int b)
{
    return ((r & 0xffU) << 16) | ((g & 0xffU) << 8) | (b & 0xffU);
}

static void delay(void)
{
    volatile unsigned long i;

    for (i = 0; i < 1800000UL; i++) {
    }
}

static int write_all(int fd, const void *buf, size_t len)
{
    const char *p = buf;

    while (len > 0) {
        ssize_t n = write(fd, p, len);

        if (n < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        if (n == 0) {
            errno = EIO;
            return -1;
        }

        p += n;
        len -= (size_t)n;
    }

    return 0;
}

static int tty_set_raw_nonblock(int fd)
{
    struct termios t;
    int flags;

    if (tcgetattr(fd, &old_termios) < 0)
        return -1;
    old_termios_valid = 1;

    t = old_termios;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN] = 0;
    t.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &t) < 0)
        return -1;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return -1;

    old_tty_flags = flags;
    old_tty_flags_valid = 1;

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;

    return 0;
}

static void tty_restore(int fd)
{
    if (old_termios_valid)
        tcsetattr(fd, TCSANOW, &old_termios);
    if (old_tty_flags_valid)
        fcntl(fd, F_SETFL, old_tty_flags);
}

static void draw_background(uint32_t *fb, unsigned int frame)
{
    unsigned int x;
    unsigned int y;

    for (y = 0; y < FB_HEIGHT; y++) {
        for (x = 0; x < FB_WIDTH; x++) {
            unsigned int band = y / 40U;
            unsigned int r = (32U + band * 19U + frame) & 0xffU;
            unsigned int g = (24U + x / 4U) & 0xffU;
            unsigned int b = (96U + y / 2U + frame * 2U) & 0xffU;

            fb[(size_t)y * FB_WIDTH + x] = rgb(r, g, b);
        }
    }
}

static void draw_rect(uint32_t *fb, int px, int py,
                      int w, int h, uint32_t color)
{
    int x;
    int y;

    for (y = 0; y < h; y++) {
        int yy = py + y;

        if (yy < 0 || yy >= FB_HEIGHT)
            continue;

        for (x = 0; x < w; x++) {
            int xx = px + x;

            if (xx < 0 || xx >= FB_WIDTH)
                continue;

            fb[(size_t)yy * FB_WIDTH + xx] = color;
        }
    }
}

static void draw_frame(uint32_t *fb, int player_x, int player_y,
                       unsigned int frame)
{
    draw_background(fb, frame);

    draw_rect(fb, player_x - 4, player_y - 4,
              PLAYER_W + 8, PLAYER_H + 8, rgb(255, 255, 255));
    draw_rect(fb, player_x, player_y,
              PLAYER_W, PLAYER_H, rgb(255, 48, 48));
    draw_rect(fb, player_x + 10, player_y + 10,
              PLAYER_W - 20, PLAYER_H - 20, rgb(255, 208, 48));
}

static int flush_frame(int fd, uint32_t *fb)
{
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -1;

    return write_all(fd, fb, FB_SIZE);
}

static int read_keys(int tty_fd, int *player_x, int *player_y)
{
    char buf[32];
    ssize_t n;
    ssize_t i;
    int quit = 0;

    for (;;) {
        n = read(tty_fd, buf, sizeof(buf));
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
                return quit;
            return -1;
        }
        if (n == 0)
            return quit;

        for (i = 0; i < n; i++) {
            switch (buf[i]) {
            case 'w':
            case 'W':
                *player_y -= PLAYER_STEP;
                break;
            case 's':
            case 'S':
                *player_y += PLAYER_STEP;
                break;
            case 'a':
            case 'A':
                *player_x -= PLAYER_STEP;
                break;
            case 'd':
            case 'D':
                *player_x += PLAYER_STEP;
                break;
            case 'q':
            case 'Q':
                quit = 1;
                break;
            default:
                break;
            }
        }
    }
}

static void clamp_player(int *x, int *y)
{
    if (*x < 0)
        *x = 0;
    if (*y < 0)
        *y = 0;
    if (*x > FB_WIDTH - PLAYER_W)
        *x = FB_WIDTH - PLAYER_W;
    if (*y > FB_HEIGHT - PLAYER_H)
        *y = FB_HEIGHT - PLAYER_H;
}

int main(void)
{
    uint32_t *fb;
    int fb_fd;
    int tty_fd;
    int player_x = (FB_WIDTH - PLAYER_W) / 2;
    int player_y = (FB_HEIGHT - PLAYER_H) / 2;
    unsigned int frame = 0;
    int ret = 0;

    fb_fd = open(FB_PATH, O_WRONLY);
    if (fb_fd < 0) {
        printf("fbkeys: open %s failed: errno=%d %s\n",
               FB_PATH, errno, strerror(errno));
        return 1;
    }

    tty_fd = open(TTY_PATH, O_RDONLY | O_NONBLOCK);
    if (tty_fd < 0) {
        printf("fbkeys: open %s failed: errno=%d %s\n",
               TTY_PATH, errno, strerror(errno));
        close(fb_fd);
        return 1;
    }

    if (tty_set_raw_nonblock(tty_fd) < 0) {
        printf("fbkeys: configure tty failed: errno=%d %s\n",
               errno, strerror(errno));
        close(tty_fd);
        close(fb_fd);
        return 1;
    }

    fb = malloc(FB_SIZE);
    if (!fb) {
        printf("fbkeys: malloc %lu bytes failed\n",
               (unsigned long)FB_SIZE);
        tty_restore(tty_fd);
        close(tty_fd);
        close(fb_fd);
        return 1;
    }

    printf("fbkeys: use WASD to move, Q to quit\n");

    for (;;) {
        int key_ret = read_keys(tty_fd, &player_x, &player_y);

        if (key_ret < 0) {
            printf("fbkeys: read key failed: errno=%d %s\n",
                   errno, strerror(errno));
            ret = 1;
            break;
        }
        if (key_ret > 0)
            break;

        clamp_player(&player_x, &player_y);
        draw_frame(fb, player_x, player_y, frame++);

        if (flush_frame(fb_fd, fb) < 0) {
            printf("fbkeys: draw failed: errno=%d %s\n",
                   errno, strerror(errno));
            ret = 1;
            break;
        }

        delay();
    }

    tty_restore(tty_fd);
    free(fb);
    close(tty_fd);
    close(fb_fd);
    printf("fbkeys: done\n");
    return ret;
}
