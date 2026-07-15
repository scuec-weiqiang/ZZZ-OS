#ifndef __OS_TTY_H
#define __OS_TTY_H

#include <os/ringbuffer.h>
#include <os/mutex.h>
#include <os/spinlock.h>
#include <os/tty_buffer.h>
#include <os/tty_driver.h>
#include <os/tty_flip.h>
#include <os/tty_ldisc.h>
#include <os/tty_port.h>
#include <os/types.h>
#include <os/wait.h>
#include <uapi/linux/termios.h>
#include <uapi/linux/tty.h>
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
    struct tty_ldisc *ldisc;
    void *disc_data;

    struct ktermios termios;
    struct winsize winsize;
    unsigned int receive_room;

    struct tty_bufhead buf;
    struct ringbuffer read_buf;

    struct wait_queue read_wait;
    struct wait_queue write_wait;

    spinlock_t lock;
    struct mutex read_lock;
    struct mutex write_lock;

    unsigned int count;
    unsigned int open_count;

    pid_t session;
    pid_t foreground_pgrp;
    pid_t pgrp;

    int echo;
    int canonical;
    unsigned int head;
    unsigned int tail;
    unsigned int inbuf_count;
    char linebuf[TTY_LINE_SIZE];
    unsigned int line_len;

    unsigned long flags;
};


extern struct ktermios tty_std_termios;

int tty_register_driver(struct tty_driver *driver);
void tty_unregister_driver(struct tty_driver *driver);
int tty_register_device(struct tty_driver *driver, int index, struct device *parent);
int tty_unregister_device(struct tty_driver *driver, int index);

void tty_port_init(struct tty_port *port);
void tty_port_destroy(struct tty_port *port);

void tty_init(struct tty_struct *tty, struct tty_driver *driver, struct tty_port *port, int index,
              void *driver_data);


void tty_buffer_init(struct tty_port *port);
void tty_buffer_free_all(struct tty_port *port);
#endif
