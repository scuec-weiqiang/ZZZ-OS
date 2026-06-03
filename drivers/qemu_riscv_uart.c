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

#define UART_TX_IDLE (1U << 5)
#define UART_RX_READY (1U << 0)

#define UART_RX_BUF_SIZE 256

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

struct uart_rx_buffer {
    spinlock_t lock;
    unsigned int head;
    unsigned int tail;
    char data[UART_RX_BUF_SIZE];
};

struct qemu_uart_info {
    virt_addr_t base;
    dev_t dev_num;
    struct cdev cdev;
    struct platform_device *pdev;
    struct wait_queue_head read_wait;
    struct irq_deferred_work rx_deferred;
    struct uart_rx_buffer rxbuf;
};

static struct qemu_uart_info *uart0;

#define UART0 ((volatile struct uart_reg *)(uart0->base))

static inline int uart_rxbuf_is_empty(void)
{
    return uart0->rxbuf.head == uart0->rxbuf.tail;
}

static inline int uart_rxbuf_is_full(void)
{
    return ((uart0->rxbuf.head + 1) % UART_RX_BUF_SIZE) == uart0->rxbuf.tail;
}

static void uart_rxbuf_push(char ch)
{
    unsigned long flags = spin_lock_irqsave(&uart0->rxbuf.lock);

    if (!uart_rxbuf_is_full()) {
        uart0->rxbuf.data[uart0->rxbuf.head] = ch;
        uart0->rxbuf.head = (uart0->rxbuf.head + 1) % UART_RX_BUF_SIZE;
    }

    spin_unlock_irqrestore(&uart0->rxbuf.lock, flags);
}

static int uart_rxbuf_pop(char *ch)
{
    unsigned long flags;
    int ok = 0;

    flags = spin_lock_irqsave(&uart0->rxbuf.lock);
    if (!uart_rxbuf_is_empty()) {
        *ch = uart0->rxbuf.data[uart0->rxbuf.tail];
        uart0->rxbuf.tail = (uart0->rxbuf.tail + 1) % UART_RX_BUF_SIZE;
        ok = 1;
    }
    spin_unlock_irqrestore(&uart0->rxbuf.lock, flags);

    return ok;
}

static inline int uart_tx_ready(void)
{
    return (UART0->LSR & UART_TX_IDLE) != 0;
}

static inline int uart_rx_ready(void)
{
    return (UART0->LSR & UART_RX_READY) != 0;
}

static void uart_putc(char c)
{
    while (!uart_tx_ready()) {
    }
    UART0->RHR_THR_DLL = (u8)c;
}

static char uart_getc_hw(void)
{
    while (!uart_rx_ready()) {
    }
    return (char)UART0->RHR_THR_DLL;
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
    UART0->FCR_ISR = 0x01;       /* enable FIFO */
}

static void uart_rx_deferred(void *arg)
{
    (void)arg;
    if (!uart_rxbuf_is_empty() && !wait_queue_empty(&uart0->read_wait)) {
        wake_up_one(&uart0->read_wait);
    }
}

static irqreturn_t uart_irq_handler(int virq, void *dev_id)
{
    (void)virq;
    (void)dev_id;

    while (uart_rx_ready()) {
        uart_rxbuf_push((char)UART0->RHR_THR_DLL);
    }

    if (!uart_rxbuf_is_empty() && !wait_queue_empty(&uart0->read_wait)) {
        irq_deferred_work_queue(&uart0->rx_deferred);
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
    size_t written = 0;

    (void)file;
    if (buf == NULL || offset == NULL) {
        return -1;
    }

    while (written < size) {
        uart_putc(buf[written]);
        written++;
    }

    *offset += written;
    return (ssize_t)written;
}

static ssize_t uart_read(struct file *file, char *buf, size_t size, loff_t *offset)
{
    size_t read = 0;

    (void)file;
    if (buf == NULL || offset == NULL) {
        return -1;
    }

    while (read < size) {
        char ch;

        if (uart_rxbuf_pop(&ch)) {
            buf[read++] = ch;
            continue;
        }

        if (uart_rx_ready()) {
            buf[read++] = uart_getc_hw();
            continue;
        }

        if (read > 0) {
            break;
        }

        sleep_on(&uart0->read_wait);
    }

    *offset += read;
    return (ssize_t)read;
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
    init_waitqueue_head(&uart0->read_wait);
    spin_lock_init(&uart0->rxbuf.lock);
    uart0->rxbuf.head = 0;
    uart0->rxbuf.tail = 0;

    uart0->base = platform_ioremap_resource(pdev, 0);
    printk("qemu_uart: base=%lx\n", (unsigned long)uart0->base);
    if (uart0->base == 0) {
        ret = -ENODEV;
        goto err_free;
    }

    uart_reg_init();
    irq_deferred_work_register(&uart0->rx_deferred, uart_rx_deferred, NULL);

    virq = platform_get_irq(pdev, 0);
    if (virq >= 0) {
        irq_request(virq, uart_irq_handler, "qemu_uart", NULL);
        irq_enable(virq);
        printk("qemu_uart: irq=%d\n", virq);
    } else {
        printk("qemu_uart: no irq found, polling mode only\n");
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
