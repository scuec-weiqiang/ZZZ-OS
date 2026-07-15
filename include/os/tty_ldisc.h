#ifndef __OS_TTY_LDISC_H
#define __OS_TTY_LDISC_H

#include <os/types.h>
#include <os/atomic.h>

#define NR_LDISCS	31

struct tty_struct;
struct file;

struct tty_ldisc {
	struct tty_ldisc_ops *ops;
	struct tty_struct *tty;
};

struct tty_ldisc_ops {
	int num;
	const char *name;
	atomic_t refcnt;

	int (*open)(struct tty_struct *tty);
	void (*close)(struct tty_struct *tty);

	ssize_t (*read)(struct tty_struct *tty, struct file *file, char *buf,
			size_t nr);
	ssize_t (*write)(struct tty_struct *tty, struct file *file,
			 const char *buf, size_t nr);

	void (*receive_buf)(struct tty_struct *tty, const unsigned char *cp,
			    const unsigned char *fp, size_t count);
	size_t (*receive_buf2)(struct tty_struct *tty, const unsigned char *cp,
			       const unsigned char *fp, size_t count);
	void (*flush_buffer)(struct tty_struct *tty);
	ssize_t (*chars_in_buffer)(struct tty_struct *tty);
	int (*ioctl)(struct tty_struct *tty, struct file *file,
		     unsigned int cmd, unsigned long arg);
};

int tty_register_ldisc(struct tty_ldisc_ops *new_ldisc);
void tty_unregister_ldisc(struct tty_ldisc_ops *ldisc);
struct tty_ldisc *tty_ldisc_get(struct tty_struct *tty, int disc);
void tty_ldisc_put(struct tty_ldisc *ld);
void tty_ldisc_flush(struct tty_struct *tty);
struct tty_ldisc *tty_ldisc_ref(struct tty_struct *tty);
void tty_ldisc_deref(struct tty_ldisc *ld);
void tty_ldisc_init(struct tty_struct *tty);
void tty_ldisc_deinit(struct tty_struct *tty);
#endif /* __OS_TTY_LDISC_H */
