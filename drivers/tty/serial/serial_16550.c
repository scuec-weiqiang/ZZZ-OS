/**
 * @FilePath     : /ZZZ-OS/drivers/tty/serial/serial_16550.c
 * @Description  : QEMU RISC-V virt UART driver
 */

#include <os/console.h>
#include <os/errno.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/kmalloc.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/serial_core.h>
#include <os/spinlock.h>
#include <os/string.h>
#include <os/wait.h>

#define UART_TX_IDLE (1U << 5)
#define UART_RX_READY (1U << 0)
#define UART_DEFAULT_CLOCK 3686400U
#define UART_LCR_DLAB (1U << 7)

struct uart_reg {
    u8 RHR_THR_DLL;
    u8 IER_DLM;
    u8 FCR_ISR;
    u8 LCR;
    u8 MCR;
    u8 LSR;
    u8 MSR;
    u8 SPR;
};

struct serial_16550_info {
    virt_addr_t base;
    struct platform_device *pdev;
    struct uart_port port;
    int virq;
    bool irq_registered;
};

static struct serial_16550_info *uart_info;

static inline volatile struct uart_reg *serial_16550_regs(struct serial_16550_info *info) {
    return (volatile struct uart_reg *)info->base;
}

static inline int uart_tx_ready(struct serial_16550_info *info) {
    return (serial_16550_regs(info)->LSR & UART_TX_IDLE) != 0;
}

static inline int uart_rx_ready(struct serial_16550_info *info) {
    return (serial_16550_regs(info)->LSR & UART_RX_READY) != 0;
}

static inline void uart_enable_rx_irq(struct serial_16550_info *info) {
    /* ns16550a IER bit0: received data available interrupt */
    serial_16550_regs(info)->IER_DLM = 0x01;
}

static unsigned int serial_16550_baud(const struct ktermios *termios)
{
    static const unsigned int baud_table[] = {
        0, 50, 75, 110, 134, 150, 200, 300,
        600, 1200, 1800, 2400, 4800, 9600, 19200, 38400,
    };
    unsigned int code = termios->c_cflag & CBAUD;

    if (termios->c_ospeed)
        return termios->c_ospeed;
    if (code < sizeof(baud_table) / sizeof(baud_table[0]))
        return baud_table[code];
    if (code == B57600)
        return 57600;
    if (code == B115200)
        return 115200;
    return 9600;
}

static void uart_putc_info(struct serial_16550_info *info, char c) {
    while (!uart_tx_ready(info)) {
    }
    serial_16550_regs(info)->RHR_THR_DLL = (u8)c;
}

static void uart_putc(char c) {
    if (uart_info != NULL) {
        uart_putc_info(uart_info, c);
    }
}

/* static void uart_puts(const char *s)
 * {
 *     while (*s) {
 *         uart_putc(*s++);
 *     }
 * }
 */

static void uart_reg_init(struct serial_16550_info *info) {
    volatile struct uart_reg *regs = serial_16550_regs(info);

    /*
     * ns16550a on QEMU virt:
     * - disable interrupts
     * - set DLAB
     * - set divisor
     * - set 8N1
     */
    regs->IER_DLM = 0x00;
    regs->LCR = UART_LCR_DLAB | 0x03;
    regs->RHR_THR_DLL = 0x18; /* 9600 baud at 3.6864 MHz */
    regs->IER_DLM = 0x00;     /* divisor high */
    regs->LCR = 0x03;
    regs->FCR_ISR = 0x07; /* enable FIFO and clear RX/TX FIFO */
}

static void serial_16550_set_termios(struct uart_port *port,
                                     struct ktermios *termios,
                                     const struct ktermios *old)
{
    struct serial_16550_info *info = port ? port->private_data : NULL;
    volatile struct uart_reg *regs;
    unsigned int baud;
    unsigned int divisor;
    unsigned int lcr;
    u8 ier;

    (void)old;
    if (!info || !termios || !port->uartclk)
        return;

    baud = serial_16550_baud(termios);
    if (!baud)
        baud = 9600;
    divisor = (port->uartclk + baud * 8U) / (baud * 16U);
    if (divisor == 0)
        divisor = 1;
    if (divisor > 0xffffU)
        divisor = 0xffffU;

    switch (termios->c_cflag & CSIZE) {
    case CS5: lcr = 0; break;
    case CS6: lcr = 1; break;
    case CS7: lcr = 2; break;
    default:  lcr = 3; break;
    }
    if (termios->c_cflag & CSTOPB)
        lcr |= 1U << 2;
    if (termios->c_cflag & PARENB) {
        lcr |= 1U << 3;
        if (!(termios->c_cflag & PARODD))
            lcr |= 1U << 4;
    }

    regs = serial_16550_regs(info);
    ier = regs->IER_DLM;
    regs->IER_DLM = 0;
    regs->LCR = (u8)(lcr | UART_LCR_DLAB);
    regs->RHR_THR_DLL = (u8)divisor;
    regs->IER_DLM = (u8)(divisor >> 8);
    regs->LCR = (u8)lcr;
    regs->IER_DLM = (termios->c_cflag & CREAD) ? ier : 0;

    termios->c_ispeed = baud;
    termios->c_ospeed = baud;
}

static void serial_16550_start_tx(struct uart_port *port) {
    struct serial_16550_info *info = port ? port->private_data : NULL;
    unsigned long flags;
    unsigned char ch;

    if (!info || !port->state)
        return;

    for (;;) {
        flags = spin_lock_irqsave(&port->lock);
        if (!ringbuffer_get(&port->state->tx_buf, &ch)) {
            spin_unlock_irqrestore(&port->lock, flags);
            break;
        }
        spin_unlock_irqrestore(&port->lock, flags);
        uart_putc_info(info, (char)ch);
    }

    uart_write_wakeup(port);
}

static int serial_16550_startup(struct uart_port *port) {
    struct serial_16550_info *info = port ? port->private_data : NULL;

    if (!info)
        return -ENODEV;
    uart_enable_rx_irq(info);
    return 0;
}

static void serial_16550_shutdown(struct uart_port *port) {
    struct serial_16550_info *info = port ? port->private_data : NULL;

    if (info)
        serial_16550_regs(info)->IER_DLM = 0;
}

static const struct uart_ops serial_16550_ops = {
    .start_tx = serial_16550_start_tx,
    .startup = serial_16550_startup,
    .shutdown = serial_16550_shutdown,
    .set_termios = serial_16550_set_termios,
};

static struct uart_driver serial_16550_driver = {
    .driver_name = "serial 16550",
    .dev_name = "ttyS",
    .nr = 1,
};

static irqreturn_t uart_irq_handler(int virq, void *dev_id) {
    struct serial_16550_info *info = dev_id;
    bool received = false;

    (void)virq;
    if (!info)
        return IRQ_NONE;

    while (uart_rx_ready(info)) {
        uart_insert_char(&info->port,
                         serial_16550_regs(info)->RHR_THR_DLL);
        received = true;
    }

    if (received)
        uart_flip_buffer_push(&info->port);

    return received ? IRQ_HANDLED : IRQ_NONE;
}

static int uart_probe(struct platform_device *pdev) {
    int ret;
    int virq;

    printk("serial_16550: probe %s\n", pdev->name);

    uart_info = kzalloc(sizeof(*uart_info));
    if (uart_info == NULL) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, uart_info);
    uart_info->pdev = pdev;
    uart_info->virq = -1;

    uart_info->base = platform_ioremap_resource(pdev, 0);
    printk("serial_16550: base=%lx\n", (unsigned long)uart_info->base);
    if (uart_info->base == 0) {
        ret = -ENODEV;
        goto err_free;
    }

    uart_port_init(&uart_info->port);
    uart_info->port.line = 0;
    uart_info->port.uartclk = of_get_u32(pdev->dev.of_node,
                                        "clock-frequency",
                                        UART_DEFAULT_CLOCK);
    uart_info->port.dev = &pdev->dev;
    uart_info->port.private_data = uart_info;
    uart_info->port.ops = &serial_16550_ops;
    uart_reg_init(uart_info);

    ret = uart_register_driver(&serial_16550_driver);
    if (ret) {
        goto err_unmap;
    }

    virq = platform_get_irq(pdev, 0);
    if (virq < 0) {
        ret = virq;
        goto err_unregister_uart;
    }
    ret = irq_request(virq, uart_irq_handler, "serial_16550", uart_info);
    if (ret)
        goto err_unregister_uart;
    uart_info->virq = virq;
    uart_info->irq_registered = true;
    uart_info->port.irq = (unsigned int)virq;
    irq_enable(virq);

    ret = uart_add_one_port(&serial_16550_driver, &uart_info->port);
    if (ret) {
        goto err_free_irq;
    }
    printk("serial_16550: irq=%d clock=%u\n", virq,
           uart_info->port.uartclk);

    console_register(uart_putc);
    printk("serial_16550: registered console and uart port\n");
    return 0;

err_free_irq:
    irq_free(uart_info->virq, uart_info);
    uart_info->irq_registered = false;
err_unregister_uart:
    uart_unregister_driver(&serial_16550_driver);
err_unmap:
    if (uart_info->base) {
        iounmap(uart_info->base, sizeof(struct uart_reg));
        uart_info->base = 0;
    }
err_free:
    kfree(uart_info);
    uart_info = NULL;
    return ret;
}

static int uart_remove(struct platform_device *pdev) {
    struct serial_16550_info *info = platform_get_drvdata(pdev);

    (void)pdev;
    if (!info) {
        return 0;
    }

    serial_16550_shutdown(&info->port);
    if (info->irq_registered) {
        irq_free(info->virq, info);
        info->irq_registered = false;
    }

    uart_remove_one_port(&serial_16550_driver, &info->port);
    uart_unregister_driver(&serial_16550_driver);

    if (info->base) {
        iounmap(info->base, sizeof(struct uart_reg));
        info->base = 0;
    }

    kfree(info);
    uart_info = NULL;
    return 0;
}

static const struct of_device_id uart_of_match[] = {
    {.compatible = "wq,uart"},
    {/* sentinel */},
};

static struct platform_driver serial_16550_platform_driver = {
    .name = "serial 16550",
    .probe = uart_probe,
    .remove = uart_remove,
    .driver = {
            .of_match_table = uart_of_match,
        },
};

static int serial_16550_init(void)
{
    int ret;

    printk("serial_16550: registering platform driver\n");
    ret = platform_driver_register(&serial_16550_platform_driver);
    if (ret)
        printk("serial_16550: driver registration failed: %d\n", ret);
    return ret;
}

static void serial_16550_exit(void)
{
    platform_driver_unregister(&serial_16550_platform_driver);
}

core_initcall(serial_16550_init);
module_exit(serial_16550_exit, ".exitcall");
