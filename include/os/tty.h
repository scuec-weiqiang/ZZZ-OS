#ifndef __OS_TTY_H
#define __OS_TTY_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/wait.h>
#include <uapi/asm-generic/termios.h>
#include <os/tty_buffer.h>
#include <os/tty_port.h>
#include <os/tty_driver.h>
#include <uapi/linux/tty.h>

#define TTY_LINE_SIZE 256

struct file;
struct tty_driver;
struct tty_ldisc_ops;
struct tty_port;

struct tty_struct {
    spinlock_t lock;

    int index;
    dev_t dev;
    int count;
    
    unsigned int receive_room;

    struct tty_driver *driver;
    struct tty_port *port;
    struct tty_ldisc *ldisc;

    struct ktermios termios;
    struct winsize winsize;

    pid_t pgrp;
    pid_t session;

    // char inbuf[TTY_BUF_SIZE];
    unsigned int head;
    unsigned int tail;
    unsigned int inbuf_count;

    char linebuf[TTY_LINE_SIZE];
    unsigned int line_len;

    int echo;
    int canonical;

    void *driver_data;
};




int tty_register_driver(struct tty_driver *driver);
void tty_unregister_driver(struct tty_driver *driver);
int tty_register_device(struct tty_driver *driver, int index);
void tty_port_init(struct tty_port *port);
void tty_init(struct tty_struct *tty, struct tty_driver *driver,
              struct tty_port *port, int index, void *driver_data);
void tty_receive_char(struct tty_struct *tty, char ch);
ssize_t tty_read(struct tty_struct *tty, char *buf, size_t size);
ssize_t tty_write(struct tty_struct *tty, const char *buf, size_t size);
long tty_ioctl(struct tty_struct *tty, unsigned long request, unsigned long arg);

#endif
