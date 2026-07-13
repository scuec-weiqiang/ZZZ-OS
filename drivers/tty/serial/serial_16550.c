/**
 * @FilePath     : /ZZZ-OS/drivers/qemu_riscv_uart.c
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
    regs->LCR |= (1U << 7);
    regs->RHR_THR_DLL = 0x03; /* divisor low */
    regs->IER_DLM = 0x00;     /* divisor high */
    regs->LCR &= ~(1U << 7);
    regs->LCR = (regs->LCR & ~0x03U) | 0x03U;
    regs->LCR &= ~(1U << 2);
    regs->FCR_ISR = 0x07; /* enable FIFO and clear RX/TX FIFO */
}

static void serial_16550_put_char(struct uart_port *port, char ch) {
    struct serial_16550_info *info = port ? port->private_data : uart_info;

    if (info != NULL) {
        uart_putc_info(info, ch);
    }
}

static const struct uart_ops serial_16550_ops = {
    .put_char = serial_16550_put_char,
};

static struct uart_driver serial_16550_driver = {
    .driver_name = "serial 16550",
    .dev_name = "ttyS",
    .nr = 1,
};

static irqreturn_t uart_irq_handler(int virq, void *dev_id) {
    (void)virq;
    (void)dev_id;

    while (uart_rx_ready(uart_info)) {
        uart_receive_char(&uart_info->port, (char)serial_16550_regs(uart_info)->RHR_THR_DLL);
    }

    return IRQ_HANDLED;
}

static int uart_probe(struct platform_device *pdev) {
    int ret;
    int virq;

    uart_info = kzalloc(sizeof(*uart_info));
    if (uart_info == NULL) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, uart_info);
    uart_info->pdev = pdev;

    uart_info->base = platform_ioremap_resource(pdev, 0);
    printk("serial_16550: base=%lx\n", (unsigned long)uart_info->base);
    if (uart_info->base == 0) {
        ret = -ENODEV;
        goto err_free;
    }

    uart_reg_init(uart_info);
    uart_info->port.line = 0;
    uart_info->port.private_data = uart_info;
    uart_info->port.ops = &serial_16550_ops;

    ret = uart_register_driver(&serial_16550_driver);
    if (ret && ret != -EBUSY) {
        goto err_unmap;
    }

    ret = uart_add_one_port(&serial_16550_driver, &uart_info->port);
    if (ret) {
        goto err_unregister_uart;
    }

    uart_enable_rx_irq(uart_info);

    virq = platform_get_irq(pdev, 0);
    if (virq >= 0) {
        if (irq_request(virq, uart_irq_handler, "serial_16550", NULL) == 0) {
            irq_enable(virq);
            printk("serial_16550: irq=%d\n", virq);
        } else {
            printk("serial_16550: irq=%d request failed\n", virq);
        }
    } else {
        printk("serial_16550: no irq found\n");
    }

    console_register(uart_putc);
    printk("serial_16550: registered console and uart port\n");
    return 0;

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
    .driver =
        {
            .of_match_table = uart_of_match,
        },
};

module_platform_driver(serial_16550_platform_driver);
