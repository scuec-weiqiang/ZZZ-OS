/**
 * @FilePath     : /ZZZ-OS/include/os/tty_driver.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-14 01:08:54
 * @LastEditTime : 2026-07-15 17:13:45
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __OS_TTY_DRIVER_H
#define __OS_TTY_DRIVER_H

#include <os/types.h>
#include <fs/cdev.h>
#include <uapi/linux/termios.h>
#include <os/err.h>
#include <os/device.h>

struct tty_driver;
struct tty_struct;
struct file;

struct tty_operations {
	int  (*install)(struct tty_driver *driver, struct tty_struct *tty);
	void (*remove)(struct tty_driver *driver, struct tty_struct *tty);
	int  (*open)(struct tty_struct * tty, struct file * filp);
	void (*close)(struct tty_struct * tty, struct file * filp);
	ssize_t (*write)(struct tty_struct *tty, const u8 *buf, size_t count);
	int (*write_room)(struct tty_struct *tty);
	void (*set_termios)(struct tty_struct *tty, const struct ktermios *old);
	int  (*chars_in_buffer)(struct tty_struct *tty);
	void (*flush_buffer)(struct tty_struct *tty);
};

struct tty_driver {
    const char *driver_name;
    const char *name;                 // "ttyS"
    struct cdev **cdevs;
    struct device **devices;
    int major;
    int minor_start;
    int num;                   // 设备数量

    struct ktermios init_termios; /* Initial termios */

    struct tty_struct **ttys;  // 每个 index 对应一个 tty
    struct tty_port **ports;   // 每个 index 对应一个 port
    struct ktermios *termios;  // 每个 index 对应一个 termios
    void* driver_state; // 驱动私有数据

    const struct tty_operations *ops;
    struct list_head tty_drivers;
};

extern struct list_head tty_drivers;

void tty_put_driver(struct tty_driver *driver);
struct tty_driver *tty_alloc_driver(unsigned int lines);

static inline struct tty_driver *alloc_tty_driver(unsigned int lines)
{
	return tty_alloc_driver(lines);
}

static inline void tty_set_operations(struct tty_driver *driver,
		const struct tty_operations *op)
{
	driver->ops = op;
}

/* tty driver types */
#define TTY_DRIVER_TYPE_SYSTEM		0x0001
#define TTY_DRIVER_TYPE_CONSOLE		0x0002
#define TTY_DRIVER_TYPE_SERIAL		0x0003
#define TTY_DRIVER_TYPE_PTY		0x0004
#define TTY_DRIVER_TYPE_SCC		0x0005	/* scc driver */
#define TTY_DRIVER_TYPE_SYSCONS		0x0006
#endif /* __OS_TTY_DRIVER_H */
