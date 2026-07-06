#ifndef __OS_SERIAL_CORE_H
#define __OS_SERIAL_CORE_H
#include <os/types.h>
#include <os/tty_driver.h>
#include <os/tty.h>

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

#endif