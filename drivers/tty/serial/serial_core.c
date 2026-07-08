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

static const struct tty_operations uart_tty_ops = {
    .write = uart_tty_write,
    .set_termios = uart_tty_set_termios,
};

void uart_port_init(struct uart_port *port) {
    if (port == NULL) {
        return;
    }

    spin_lock_init(&port->lock);
    tty_port_init(&port->tty_port);
    port->tty_port.driver_data = port;
}

int uart_register_driver(struct uart_driver *driver) {
    if (driver == NULL || driver->nr == 0) {
        return -EINVAL;
    }

    if (driver->state != NULL) {
        return -EBUSY;
    }

    driver->state = kzalloc(sizeof(*driver->state) * driver->nr);
    if (driver->state == NULL) {
        return -ENOMEM;
    }

    driver->tty_driver = kzalloc(sizeof(*driver->tty_driver));
    if (driver->tty_driver == NULL) {
        goto err_free_state;
    }

    driver->tty_driver->ttys = kzalloc(sizeof(*driver->tty_driver->ttys) * driver->nr);
    if (driver->tty_driver->ttys == NULL) {
        goto err_free_tty_driver;
    }

    driver->tty_driver->ports = kzalloc(sizeof(*driver->tty_driver->ports) * driver->nr);
    if (driver->tty_driver->ports == NULL) {
        goto err_free_ttys;
    }

    driver->tty_driver->name = driver->dev_name;
    driver->tty_driver->num = driver->nr;
    driver->tty_driver->ops = &uart_tty_ops;

    return tty_register_driver(driver->tty_driver);

err_free_ttys:
    kfree(driver->tty_driver->ttys);
err_free_tty_driver:
    kfree(driver->tty_driver);
    driver->tty_driver = NULL;
err_free_state:
    kfree(driver->state);
    driver->state = NULL;
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
