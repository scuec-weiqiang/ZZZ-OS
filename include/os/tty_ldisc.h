#ifndef __OS_TTY_LDISC_H
#define __OS_TTY_LDISC_H

#include <os/types.h>

#define NR_LDISCS	31

struct tty_struct;
struct ktermios;


struct tty_ldisc {
	struct tty_ldisc_ops *ops;
	struct tty_struct *tty;
};

struct tty_ldisc_ops {
    const char *name;
    int num;
    atomic_t refcnt;

    int (*open)(struct tty_struct *tty);
    void (*close)(struct tty_struct *tty);
    void	(*flush_buffer)(struct tty_struct *tty);
    ssize_t (*read)(struct tty_struct *tty, char *buf, size_t size);
    ssize_t (*write)(struct tty_struct *tty, const char *buf, size_t size);
    void (*receive_buf)(struct tty_struct *tty,
                        const u8 *cp,
                        const unsigned char *fp,
                        size_t count);
    void (*set_termios)(struct tty_struct *tty,
                        const struct ktermios *old);
    size_t	(*receive_buf2)(struct tty_struct *tty, const u8 *cp,
			const u8 *fp, size_t count);
};



#endif /* __OS_TTY_LDISC_H */