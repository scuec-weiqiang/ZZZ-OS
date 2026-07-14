#ifndef __OS_TTY_LDISC_H
#define __OS_TTY_LDISC_H

#include <os/types.h>

#define NR_LDISCS	31

struct tty_struct;

struct tty_ldisc {
	struct tty_ldisc_ops *ops;
	struct tty_struct *tty;
};

struct tty_ldisc_ops {
    ssize_t (*read)(struct tty_struct *,
                    char __user *,
                    size_t);

    ssize_t (*write)(struct tty_struct *,
                     const char *,
                     size_t);

    void (*receive_buf)(struct tty_struct *,
                        const unsigned char *,
                        size_t);
};



#endif /* __OS_TTY_LDISC_H */