#include <os/errno.h>
#include <os/kmalloc.h>
#include <os/serial_core.h>

static ssize_t uart_tty_write(struct tty_struct *tty, const char *buf, size_t size) {
    if (tty == NULL) {
        return -EINVAL;
    }

    return uart_write((struct uart_port *)tty->driver_data, buf, size);
}

static void uart_tty_set_termios(struct tty_struct *tty, const struct ktermios *old) {
    struct uart_port *port;

    if (tty == NULL || tty->driver_data == NULL) {
        return;
    }

    port = tty->driver_data;
    if (port->ops != NULL && port->ops->set_termios != NULL) {
        port->ops->set_termios(port, old);
    }
}

static const struct tty_operations uart_ops = {
// 	.open		= uart_open,
// 	.close		= uart_close,
// 	.write		= uart_write,
// 	.put_char	= uart_put_char,
// 	.flush_chars	= uart_flush_chars,
// 	.write_room	= uart_write_room,
// 	.chars_in_buffer= uart_chars_in_buffer,
// 	.flush_buffer	= uart_flush_buffer,
// 	.ioctl		= uart_ioctl,
// 	.throttle	= uart_throttle,
// 	.unthrottle	= uart_unthrottle,
// 	.send_xchar	= uart_send_xchar,
// 	.set_termios	= uart_set_termios,
// 	.set_ldisc	= uart_set_ldisc,
// 	.stop		= uart_stop,
// 	.start		= uart_start,
// 	.hangup		= uart_hangup,
// 	.break_ctl	= uart_break_ctl,
// 	.wait_until_sent= uart_wait_until_sent,
// #ifdef CONFIG_PROC_FS
// 	.proc_fops	= &uart_proc_fops,
// #endif
// 	.tiocmget	= uart_tiocmget,
// 	.tiocmset	= uart_tiocmset,
// 	.get_icount	= uart_get_icount,
// #ifdef CONFIG_CONSOLE_POLL
// 	.poll_init	= uart_poll_init,
// 	.poll_get_char	= uart_poll_get_char,
// 	.poll_put_char	= uart_poll_put_char,
// #endif
};
static const struct tty_port_operations uart_port_ops = {
	// .activate	= uart_port_activate,
	// .shutdown	= uart_port_shutdown,
	// .carrier_raised = uart_carrier_raised,
	// .dtr_rts	= uart_dtr_rts,
};
void uart_port_init(struct uart_port *port) {
    if (port == NULL) {
        return;
    }

    spin_lock_init(&port->lock);
    tty_port_init(&port->tty_port);
    port->tty_port.driver_data = port;
}
/**
 *	uart_register_driver - register a driver with the uart core layer
 *	@drv: low level driver structure
 *
 *	Register a uart driver with the core driver.  We in turn register
 *	with the tty layer, and initialise the core driver per-port state.
 *
 *	We have a proc file in /proc/tty/driver which is named after the
 *	normal driver.
 *
 *	drv->port should be NULL, and the per-port structures should be
 *	registered using uart_add_one_port after this call has succeeded.
 */
int uart_register_driver(struct uart_driver *drv)
{
	struct tty_driver *normal;
	int i, retval;

	/*
	 * Maybe we should be using a slab cache for this, especially if
	 * we have a large number of ports to handle.
	 */
	drv->state = kzalloc(sizeof(struct uart_state) * drv->nr);
	if (!drv->state)
		goto out;

	normal = alloc_tty_driver(drv->nr);
	if (!normal)
		goto out_kfree;

	drv->tty_driver = normal;

	normal->driver_name	= drv->driver_name;
	normal->name		= drv->dev_name;
	normal->major		= drv->major;
	normal->minor_start	= drv->minor;
	normal->type		= TTY_DRIVER_TYPE_SERIAL;
	// normal->subtype		= SERIAL_TYPE_NORMAL;
	normal->init_termios	= tty_std_termios;
	normal->init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	normal->init_termios.c_ispeed = normal->init_termios.c_ospeed = 9600;
	// normal->flags		= TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
	normal->driver_state    = drv;
	tty_set_operations(normal, &uart_ops);

	/*
	 * Initialise the UART state(s).
	 */
	for (i = 0; i < drv->nr; i++) {
		struct uart_state *state = drv->state + i;
		struct tty_port *port = &state->port;

		tty_port_init(port);
		port->ops = &uart_port_ops;
	}

	retval = tty_register_driver(normal);
	if (retval >= 0)
		return retval;

	for (i = 0; i < drv->nr; i++)
		tty_port_destroy(&drv->state[i].port);
	put_tty_driver(normal);
out_kfree:
	kfree(drv->state);
out:
	return -ENOMEM;
}


void uart_unregister_driver(struct uart_driver *driver) {
    if (driver == NULL) {
        return;
    }

    if (driver->tty_driver != NULL) {
        tty_unregister_driver(driver->tty_driver);
    }

    if (driver->state != NULL) {
        kfree(driver->state);
        driver->state = NULL;
    }

    if (driver->tty_driver != NULL) {
        if (driver->tty_driver->ttys != NULL) {
            kfree(driver->tty_driver->ttys);
        }
        if (driver->tty_driver->ports != NULL) {
            kfree(driver->tty_driver->ports);
        }
        kfree(driver->tty_driver);
        driver->tty_driver = NULL;
    }
}

int uart_add_one_port(struct uart_driver *driver, struct uart_port *port) {
    struct uart_state *state;
    int ret;

    if (driver == NULL || port == NULL || port->ops == NULL) {
        return -EINVAL;
    }

    if (driver->state == NULL || driver->tty_driver == NULL || port->line >= driver->nr) {
        return -EINVAL;
    }

    if (driver->state[port->line].port != NULL) {
        return -EBUSY;
    }

    state = &driver->state[port->line];
    uart_port_init(port);
    port->uart_driver = driver;
    state->port = port;
    tty_init(&state->tty, driver->tty_driver, &port->tty_port, (int)port->line, port);
    driver->tty_driver->ttys[port->line] = &state->tty;
    driver->tty_driver->ports[port->line] = &port->tty_port;

    ret = tty_register_device(driver->tty_driver, (int)port->line);
    if (ret) {
        driver->tty_driver->ttys[port->line] = NULL;
        driver->tty_driver->ports[port->line] = NULL;
        state->port = NULL;
        port->uart_driver = NULL;
        port->tty_port.tty = NULL;
        return ret;
    }

    if (port->ops->startup != NULL) {
        ret = port->ops->startup(port);

        if (ret) {
            driver->tty_driver->ttys[port->line] = NULL;
            driver->tty_driver->ports[port->line] = NULL;
            state->port = NULL;
            port->uart_driver = NULL;
        }
        return ret;
    }

    return 0;
}

void uart_remove_one_port(struct uart_driver *driver, struct uart_port *port) {
    if (driver == NULL || port == NULL || driver->state == NULL) {
        return;
    }

    if (port->ops != NULL && port->ops->shutdown != NULL) {
        port->ops->shutdown(port);
    }

    if (port->line < driver->nr && driver->state[port->line].port == port) {
        driver->state[port->line].port = NULL;
        if (driver->tty_driver != NULL) {
            driver->tty_driver->ttys[port->line] = NULL;
            driver->tty_driver->ports[port->line] = NULL;
        }
    }

    port->tty_port.tty = NULL;
    port->uart_driver = NULL;
}

void uart_write_char(struct uart_port *port, char ch) {
    if (port == NULL || port->ops == NULL || port->ops->put_char == NULL) {
        return;
    }

    port->ops->put_char(port, ch);
}

ssize_t uart_write(struct uart_port *port, const char *buf, size_t size) {
    size_t written;

    if (port == NULL || buf == NULL) {
        return -EINVAL;
    }

    for (written = 0; written < size; written++) {
        uart_write_char(port, buf[written]);
    }

    return (ssize_t)written;
}

void uart_receive_char(struct uart_port *port, char ch) {
    if (port == NULL || port->tty_port.tty == NULL) {
        return;
    }

    tty_receive_char(port->tty_port.tty, ch);
}

struct tty_struct *uart_port_tty(struct uart_port *port) {
    if (port == NULL) {
        return NULL;
    }

    return port->tty_port.tty;
}
