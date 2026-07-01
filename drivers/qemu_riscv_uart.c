/**
 * @FilePath     : /ZZZ-OS/drivers/qemu_riscv_uart.c
 * @Description  : QEMU RISC-V virt UART driver
 */

#include <fs/cdev.h>
#include <os/console.h>
#include <os/irq.h>
#include <os/irqreturn.h>
#include <os/kmalloc.h>
#include <os/errno.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/sched.h>
#include <os/spinlock.h>
#include <os/wait.h>
#include <os/string.h>
#include <os/tty.h>

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

struct qemu_uart_info {
    virt_addr_t base;
    dev_t dev_num;
    struct cdev cdev;
    struct platform_device *pdev;
    struct tty tty;
};

static struct qemu_uart_info *uart0;

#define UART0 ((volatile struct uart_reg *)(uart0->base))

static inline int uart_tx_ready(void)
{
    return (UART0->LSR & UART_TX_IDLE) != 0;
}

static inline int uart_rx_ready(void)
{
    return (UART0->LSR & UART_RX_READY) != 0;
}

static inline void uart_enable_rx_irq(void)
{
    /* ns16550a IER bit0: received data available interrupt */
    UART0->IER_DLM = 0x01;
}

static void uart_putc(char c)
{
    while (!uart_tx_ready()) {
    }
    UART0->RHR_THR_DLL = (u8)c;
}

/* static void uart_puts(const char *s)
 * {
 *     while (*s) {
 *         uart_putc(*s++);
 *     }
 * }
 */

static void uart_reg_init(void)
{
    /*
     * ns16550a on QEMU virt:
     * - disable interrupts
     * - set DLAB
     * - set divisor
     * - set 8N1
     */
    UART0->IER_DLM = 0x00;
    UART0->LCR |= (1U << 7);
    UART0->RHR_THR_DLL = 0x03;   /* divisor low */
    UART0->IER_DLM = 0x00;       /* divisor high */
    UART0->LCR &= ~(1U << 7);
    UART0->LCR = (UART0->LCR & ~0x03U) | 0x03U;
    UART0->LCR &= ~(1U << 2);
    UART0->FCR_ISR = 0x07;       /* enable FIFO and clear RX/TX FIFO */
}

static void uart_tty_putc(char ch, void *data)
{
    (void)data;
    uart_putc(ch);
}

static irqreturn_t uart_irq_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    while (uart_rx_ready()) {
        tty_receive_char(&uart0->tty, (char)UART0->RHR_THR_DLL);
    }

    return IRQ_HANDLED;
}

static int uart_open(struct inode *inode, struct file *file)
{
    (void)inode;
    file->private_data = uart0;
    return 0;
}

static int uart_release(struct inode *inode, struct file *file)
{
    (void)inode;
    file->private_data = NULL;
    return 0;
}
static ssize_t uart_write(struct file *file, const char *buf, size_t size, loff_t *offset)
{
    ssize_t written;

    (void)file;
    if (buf == NULL || offset == NULL) {
        return -1;
    }

    written = tty_write(&uart0->tty, buf, size);
    if (written < 0) {
        return written;
    }

    *offset += written;
    return written;
}

static ssize_t uart_read(struct file *file, char *buf, size_t size, loff_t *offset)
{
    ssize_t read;

    (void)file;
    if (buf == NULL || offset == NULL) {
        return -1;
    }

    read = tty_read(&uart0->tty, buf, size);
    if (read < 0) {
        return read;
    }

    *offset += read;
    return read;
}

static const struct file_operations uart_file_ops = {
    .open = uart_open,
    .release = uart_release,
    .read = uart_read,
    .write = uart_write,
};

static int uart_probe(struct platform_device *pdev)
{
    int ret;
    int virq;

    uart0 = kzalloc(sizeof(*uart0));
    if (uart0 == NULL) {
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, uart0);
    uart0->pdev = pdev;

    uart0->base = platform_ioremap_resource(pdev, 0);
    printk("qemu_uart: base=%lx\n", (unsigned long)uart0->base);
    if (uart0->base == 0) {
        ret = -ENODEV;
        goto err_free;
    }

    uart_reg_init();
    tty_init(&uart0->tty, uart_tty_putc, uart0);
    uart_enable_rx_irq();

    virq = platform_get_irq(pdev, 0);
    if (virq >= 0) {
        if (irq_request(virq, uart_irq_handler, "qemu_uart", NULL) == 0) {
            irq_enable(virq);
            printk("qemu_uart: irq=%d\n", virq);
        } else {
            printk("qemu_uart: irq=%d request failed\n", virq);
        }
    } else {
        printk("qemu_uart: no irq found\n");
    }

    ret = alloc_chrdev_region(&uart0->dev_num, 1);
    if (ret) {
        goto err_unmap;
    }

    ret = cdev_register("uart0", uart0->dev_num, &uart_file_ops, uart0);
    if (ret) {
        goto err_unmap;
    }

    console_register(uart_putc);
    printk("qemu_uart: registered console and cdev\n");
    return 0;

err_unmap:
    if (uart0->base) {
        iounmap(uart0->base, sizeof(struct uart_reg));
        uart0->base = 0;
    }
err_free:
    kfree(uart0);
    uart0 = NULL;
    return ret;
}

static int uart_remove(struct platform_device *pdev)
{
    struct qemu_uart_info *info = platform_get_drvdata(pdev);

    (void)pdev;
    if (!info) {
        return 0;
    }

    if (info->base) {
        iounmap(info->base, sizeof(struct uart_reg));
        info->base = 0;
    }

    kfree(info);
    uart0 = NULL;
    return 0;
}

static const struct of_device_id uart_of_match[] = {
    { .compatible = "wq,uart" },
    { /* sentinel */ },
};

static struct platform_driver uart_driver = {
    .name = "qemu_riscv_uart",
    .probe = uart_probe,
    .remove = uart_remove,
    .driver = {
        .of_match_table = uart_of_match,
    },
};

module_platform_driver(uart_driver);
