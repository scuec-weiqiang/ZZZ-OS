#ifndef __OS_TTY_H
#define __OS_TTY_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/wait.h>

#define TTY_BUF_SIZE 512
#define TTY_LINE_SIZE 256

struct tty {
    spinlock_t lock;

    char inbuf[TTY_BUF_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int count;

    char linebuf[TTY_LINE_SIZE];
    unsigned int line_len;

    struct wait_queue_head read_wait;

    int echo;
    int canonical;

    void (*putc)(char ch, void *data);
    void *driver_data;
};

void tty_init(struct tty *tty, void (*putc)(char ch, void *data), void *data);
void tty_receive_char(struct tty *tty, char ch);
ssize_t tty_read(struct tty *tty, char *buf, size_t size);
ssize_t tty_write(struct tty *tty, const char *buf, size_t size);

#endif
