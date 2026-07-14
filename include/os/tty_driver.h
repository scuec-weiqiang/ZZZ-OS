/**
 * @FilePath     : /ZZZ-OS/include/os/tty_driver.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-14 01:08:54
 * @LastEditTime : 2026-07-14 01:35:47
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __OS_TTY_DRIVER_H
#define __OS_TTY_DRIVER_H

#include <os/types.h>
#include <fs/cdev.h>
#include <uapi/linux/termios.h>
#include <os/err.h>

struct tty_struct;
struct file;

struct tty_operations {
    int (*open)(struct tty_struct *tty, struct file *file);
    void (*close)(struct tty_struct *tty, struct file *file);
    ssize_t (*write)(struct tty_struct *tty, const char *buf, size_t count);
    int (*write_room)(struct tty_struct *tty);
    int (*chars_in_buffer)(struct tty_struct *tty);
    void (*flush_buffer)(struct tty_struct *tty);
    int (*ioctl)(struct tty_struct *tty, unsigned int cmd, unsigned long arg);
    void (*set_termios)(struct tty_struct *tty, const struct ktermios *old);
    void (*throttle)(struct tty_struct *tty);
    void (*unthrottle)(struct tty_struct *tty);
};

struct tty_driver {
    const char *driver_name;          // "ttyS"
    const char *name;                 // "ttyS"
    int	name_base;	/* offset of printed name */
    int major;
    int minor_start;
    int num;                   // 设备数量
    short	type;		/* type of tty driver */
	// short	subtype;	/* subtype of tty driver */

    struct ktermios init_termios; /* Initial termios */

    struct tty_struct **ttys;  // 每个 index 对应一个 tty
    struct tty_port **ports;   // 每个 index 对应一个 port
    struct ktermios *termios;  // 每个 index 对应一个 termios
    void* driver_state; // 驱动私有数据

    const struct tty_operations *ops;
    struct list_head tty_drivers;
};

extern struct list_head tty_drivers;
/* Use TTY_DRIVER_* flags below */
#define tty_alloc_driver(lines, flags) \
		__tty_alloc_driver(lines, THIS_MODULE, flags)

/*
 * DEPRECATED Do not use this in new code, use tty_alloc_driver instead.
 * (And change the return value checks.)
 */
static inline struct tty_driver *alloc_tty_driver(unsigned int lines)
{
	struct tty_driver *ret = tty_alloc_driver(lines, 0);
	if (IS_ERR(ret))
		return NULL;
	return ret;
}

/* tty driver types */
#define TTY_DRIVER_TYPE_SYSTEM		0x0001
#define TTY_DRIVER_TYPE_CONSOLE		0x0002
#define TTY_DRIVER_TYPE_SERIAL		0x0003
#define TTY_DRIVER_TYPE_PTY		0x0004
#define TTY_DRIVER_TYPE_SCC		0x0005	/* scc driver */
#define TTY_DRIVER_TYPE_SYSCONS		0x0006
#endif /* __OS_TTY_DRIVER_H */