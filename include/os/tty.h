#ifndef __OS_TTY_H
#define __OS_TTY_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/wait.h>

#define TTY_BUF_SIZE 512
#define TTY_LINE_SIZE 256

#define TTY_NCCS 19

#define TTY_TCGETS 0x5401
#define TTY_TCSETS 0x5402
#define TTY_TCSETSW 0x5403
#define TTY_TCSETSF 0x5404
#define TTY_TIOCSCTTY 0x540e
#define TTY_TIOCGPGRP 0x540f
#define TTY_TIOCSPGRP 0x5410
#define TTY_TIOCGWINSZ 0x5413
#define TTY_TIOCSWINSZ 0x5414

#define TTY_VINTR 0
#define TTY_VQUIT 1
#define TTY_VERASE 2
#define TTY_VKILL 3
#define TTY_VEOF 4
#define TTY_VTIME 5
#define TTY_VMIN 6

#define TTY_ICRNL 0x00000100
#define TTY_IXON 0x00000400
#define TTY_OPOST 0x00000001
#define TTY_CS8 0x00000030
#define TTY_CREAD 0x00000080
#define TTY_HUPCL 0x00000400
#define TTY_ISIG 0x00000001
#define TTY_ICANON 0x00000002
#define TTY_ECHO 0x00000008
#define TTY_ECHOE 0x00000010
#define TTY_ECHOK 0x00000020
#define TTY_ECHOCTL 0x00000200
#define TTY_ECHOKE 0x00000800
#define TTY_IEXTEN 0x00008000

struct linux_termios {
    unsigned int c_iflag;
    unsigned int c_oflag;
    unsigned int c_cflag;
    unsigned int c_lflag;
    unsigned char c_line;
    unsigned char c_cc[TTY_NCCS];
};

struct linux_winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};

struct file;

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
    int pgrp;
    struct linux_termios termios;
    struct linux_winsize winsize;

    void (*putc)(char ch, void *data);
    void *driver_data;
};

void tty_init(struct tty *tty, void (*putc)(char ch, void *data), void *data);
void tty_receive_char(struct tty *tty, char ch);
ssize_t tty_read(struct tty *tty, char *buf, size_t size);
ssize_t tty_write(struct tty *tty, const char *buf, size_t size);
long tty_ioctl(struct tty *tty, unsigned long request, unsigned long arg);

#endif
