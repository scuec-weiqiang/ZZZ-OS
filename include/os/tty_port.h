#ifndef __OS_TTY_PORT_H
#define __OS_TTY_PORT_H

#include <os/spinlock.h>
#include <os/wait.h>

#include <os/tty_buffer.h>

struct tty_port;
struct tty_struct;

/**
 * struct tty_port_operations -- operations on tty_port
 * @carrier_raised: return true if the carrier is raised on @port
 * @dtr_rts: raise the DTR line if @active is true, otherwise lower DTR
 * @shutdown: called when the last close completes or a hangup finishes IFF the
 *	port was initialized. Do not use to free resources. Turn off the device
 *	only. Called under the port mutex to serialize against @activate and
 *	@shutdown.
 * @activate: called under the port mutex from tty_port_open(), serialized using
 *	the port mutex. Supposed to turn on the device.
 *
 *	FIXME: long term getting the tty argument *out* of this would be good
 *	for consoles.
 *
 * @destruct: called on the final put of a port. Free resources, possibly incl.
 *	the port itself.
 */
struct tty_port_operations {
	bool (*carrier_raised)(struct tty_port *port);
	void (*dtr_rts)(struct tty_port *port, bool active);
	void (*shutdown)(struct tty_port *port);
	int (*activate)(struct tty_port *port, struct tty_struct *tty);
	void (*destruct)(struct tty_port *port);
};

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
    struct wait_queue_head open_wait;
    struct wait_queue_head close_wait;
    struct wait_queue_head read_wait;
    struct wait_queue_head write_wait;

    void *client_data;
};
#endif /* __OS_TTY_PORT_H */