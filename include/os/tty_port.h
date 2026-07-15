#ifndef __OS_TTY_PORT_H
#define __OS_TTY_PORT_H

#include <os/spinlock.h>
#include <os/wait.h>

#include <os/tty_buffer.h>

struct tty_port;
struct tty_struct;

struct tty_port_client_operations {
	size_t (*receive_buf)(struct tty_port *port, const u8 *cp, const u8 *fp,
			      size_t count);
	void (*lookahead_buf)(struct tty_port *port, const u8 *cp,
			      const u8 *fp, size_t count);
	void (*write_wakeup)(struct tty_port *port);
};

struct tty_port {
    struct tty_bufhead buf;
    struct tty_struct *tty;

	const struct tty_port_operations *ops;
	const struct tty_port_client_operations *client_ops;
	
	spinlock_t lock;
	struct mutex mutex;
	struct mutex buf_mutex;
	unsigned int open_count;
	u8 *xmit_buf;
    struct wait_queue_head open_wait;
    struct wait_queue_head close_wait;
    struct wait_queue_head read_wait;
    struct wait_queue_head write_wait;

    void *client_data;
};
#endif /* __OS_TTY_PORT_H */
