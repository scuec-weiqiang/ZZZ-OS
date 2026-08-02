#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FB_WIDTH  640
#define FB_HEIGHT 400
#define FB_BPP    32
#define FB_BYTES_PER_PIXEL (FB_BPP / 8)
#define FB_SIZE ((size_t)FB_WIDTH * FB_HEIGHT * FB_BYTES_PER_PIXEL)

static uint32_t rgb(unsigned int r, unsigned int g, unsigned int b)
{
    return ((r & 0xffU) << 16) | ((g & 0xffU) << 8) | (b & 0xffU);
}

static void delay(void)
{
    volatile unsigned long i;

    for (i = 0; i < 3000000UL; i++) {
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

static void draw_frame(uint32_t *fb, unsigned int frame)
{
    const unsigned int block_w = 96;
    const unsigned int block_h = 64;
    unsigned int block_x = (frame * 7U) % (FB_WIDTH - block_w);
    unsigned int block_y = 120 + ((frame * 3U) % 120);
    unsigned int x;
    unsigned int y;

    for (y = 0; y < FB_HEIGHT; y++) {
        for (x = 0; x < FB_WIDTH; x++) {
            unsigned int r = (x + frame * 2U) & 0xffU;
            unsigned int g = (y * 2U + frame * 3U) & 0xffU;
            unsigned int b = ((x ^ y) + frame * 5U) & 0xffU;

            fb[(size_t)y * FB_WIDTH + x] = rgb(r, g, b);
        }
    }

    for (y = block_y; y < block_y + block_h; y++) {
        for (x = block_x; x < block_x + block_w; x++) {
            if (x == block_x || x == block_x + block_w - 1 ||
                y == block_y || y == block_y + block_h - 1)
                fb[(size_t)y * FB_WIDTH + x] = rgb(255, 255, 255);
            else
                fb[(size_t)y * FB_WIDTH + x] = rgb(255, 32, 32);
        }
    }
}

int main(int argc, char **argv)
{
    const char *path = "/dev/fb0";
    unsigned int frames = 300;
    uint32_t *fb;
    int fd;
    unsigned int i;

    if (argc > 1)
        frames = (unsigned int)atoi(argv[1]);

    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("fbtest: open %s failed: errno=%d %s\n",
               path, errno, strerror(errno));
        return 1;
    }

    fb = malloc(FB_SIZE);
    if (!fb) {
        printf("fbtest: malloc %lu bytes failed\n", (unsigned long)FB_SIZE);
        close(fd);
        return 1;
    }

    printf("fbtest: writing %ux%ux%u frames to %s\n",
           FB_WIDTH, FB_HEIGHT, FB_BPP, path);

    for (i = 0; i < frames; i++) {
        draw_frame(fb, i);

        if (lseek(fd, 0, SEEK_SET) < 0) {
            printf("fbtest: lseek failed: errno=%d %s\n",
                   errno, strerror(errno));
            free(fb);
            close(fd);
            return 1;
        }

        if (write_all(fd, fb, FB_SIZE) < 0) {
            printf("fbtest: write failed: errno=%d %s\n",
                   errno, strerror(errno));
            free(fb);
            close(fd);
            return 1;
        }

        delay();
    }

    printf("fbtest: done\n");
    free(fb);
    close(fd);
    return 0;
}
//tools/deploy/run_qemu_riscv64.sh --graphics
