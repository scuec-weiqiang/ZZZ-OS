#ifndef __OS_TTY_H
#define __OS_TTY_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/wait.h>
#include <uapi/linux/termios.h>
#include <os/tty_buffer.h>
#include <os/tty_port.h>
#include <os/tty_driver.h>
#include <uapi/linux/tty.h>
#include <os/ringbuffer.h>
#define TTY_LINE_SIZE 256

struct file;
struct tty_driver;
struct tty_ldisc_ops;
struct tty_port;

struct tty_struct {
    struct tty_driver *driver;
    unsigned int index;
    const struct tty_operations *ops;
    void *driver_data;
    struct tty_port *port;


    struct termios termios;

    struct tty_bufhead buf;
    struct ringbuffer read_buf;

    struct wait_queue read_wait;
    struct wait_queue write_wait;

    spinlock_t read_lock;
    struct mutex write_lock;

    unsigned int open_count;

    pid_t session;
    pid_t foreground_pgrp;

    unsigned long flags;
};


extern struct ktermios tty_std_termios;

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
void tty_buffer_init(struct tty_port *port);
#endif
