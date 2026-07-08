#ifndef __OS_SERIAL_CORE_H
#define __OS_SERIAL_CORE_H

#include <os/spinlock.h>
#include <os/types.h>
#include <os/tty.h>

struct uart_driver;
struct uart_port;

struct uart_ops {
    int (*startup)(struct uart_port *port);
    void (*shutdown)(struct uart_port *port);
    void (*set_termios)(struct uart_port *port, const struct ktermios *old);
    void (*put_char)(struct uart_port *port, char ch);
    unsigned int (*tx_empty)(struct uart_port *port);
};

struct uart_state {
    struct uart_port *port;
    struct tty_struct tty;
};

struct uart_port {
    spinlock_t lock;

    unsigned int line;       // 0 -> ttyS0
    unsigned int irq;
    unsigned long mapbase;
    void *membase;

    struct uart_driver *uart_driver;
    const struct uart_ops *ops;

    struct tty_port tty_port;

    void *private_data;
};


struct uart_driver {
    const char *driver_name;   // "qemu_uart"
    const char *dev_name;      // "ttyS"
    unsigned int major;
    unsigned int minor;
    unsigned int nr;           // 最大端口数
    struct tty_driver *tty_driver;
    struct uart_state *state;
};

int uart_register_driver(struct uart_driver *driver);
void uart_unregister_driver(struct uart_driver *driver);
int uart_add_one_port(struct uart_driver *driver, struct uart_port *port);
void uart_remove_one_port(struct uart_driver *driver, struct uart_port *port);

void uart_port_init(struct uart_port *port);
void uart_write_char(struct uart_port *port, char ch);
ssize_t uart_write(struct uart_port *port, const char *buf, size_t size);
void uart_receive_char(struct uart_port *port, char ch);
struct tty_struct *uart_port_tty(struct uart_port *port);

#endif
