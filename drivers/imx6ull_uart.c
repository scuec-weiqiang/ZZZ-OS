/**
 * @FilePath     : /ZZZ-OS/drivers/imx6ull_uart.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-12 00:25:01
 * @LastEditTime : 2026-03-19 22:59:26
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
/**
 * @FilePath: /ZZZ-OS/drivers/uart.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:10:59
 * @LastEditTime: 2025-11-14 02:20:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <drivers/core/driver.h>
// #include <fs/chrdev.h>
// #include <fs/vfs.h>
#include <os/console.h>
#include <os/driver_model.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/string.h>
#include <os/irqreturn.h>
#include <os/irq.h>

#define __IO volatile
#define __I  volatile const

typedef struct {
    __I  uint32_t URXD;                              /**< UART Receiver Register, offset: 0x0 */
         uint8_t RESERVED_0[60];
    __IO uint32_t UTXD;                              /**< UART Transmitter Register, offset: 0x40 */
         uint8_t RESERVED_1[60];
    __IO uint32_t UCR1;                              /**< UART Control Register 1, offset: 0x80 */
    __IO uint32_t UCR2;                              /**< UART Control Register 2, offset: 0x84 */
    __IO uint32_t UCR3;                              /**< UART Control Register 3, offset: 0x88 */
    __IO uint32_t UCR4;                              /**< UART Control Register 4, offset: 0x8C */
    __IO uint32_t UFCR;                              /**< UART FIFO Control Register, offset: 0x90 */
    __IO uint32_t USR1;                              /**< UART Status Register 1, offset: 0x94 */
    __IO uint32_t USR2;                              /**< UART Status Register 2, offset: 0x98 */
    __IO uint32_t UESC;                              /**< UART Escape Character Register, offset: 0x9C */
    __IO uint32_t UTIM;                              /**< UART Escape Timer Register, offset: 0xA0 */
    __IO uint32_t UBIR;                              /**< UART BRM Incremental Register, offset: 0xA4 */
    __IO uint32_t UBMR;                              /**< UART BRM Modulator Register, offset: 0xA8 */
    __I  uint32_t UBRC;                              /**< UART Baud Rate Count Register, offset: 0xAC */
    __IO uint32_t ONEMS;                             /**< UART One Millisecond Register, offset: 0xB0 */
    __IO uint32_t UTS;                               /**< UART Test Register, offset: 0xB4 */
    __IO uint32_t UMCR;                              /**< UART RS-485 Mode Control Register, offset: 0xB8 */
  } UART_Type;

phys_addr_t uart_base = 0;

#define UART1 ((UART_Type*)uart_base)
#define UART ((UART_Type*)0x02020000)

static void putc (char c)
{
    while ((UART1->UTS >> 4) & 1) {
    }
    UART1->UTXD = c;
}

void uart_putc(char c)
{
    while ((UART->UTS >> 4) & 1) {
    }
    UART->UTXD = c;
}

static char getc(void) {
    while ((UART1->UTS >> 0) & 1) {
    }
    return UART1->UTXD & 0xFF;
}

void puts(char *s)
{
    while (*s) {
        uart_putc(*s++);
        if (*s == '\n') {
            uart_putc('\r');
        }
    }
}


static void uart_reg_init() {
        // 配置UART寄存器，设置波特率、数据位、停止位等
        // 具体配置根据芯片手册进行设置
}


static irqreturn_t uart0_iqr(int virq, void *dev_id) {
    char a = getc();
    printk("%c",a);
    if('\r'==a)
        printk("\n");
    return IRQ_HANDLED;
}

// static int uart_open(struct inode *inode, struct file *file) {
//     return 0;
// }

// static ssize_t uart_write(struct inode *inode, const void *buf, size_t size, loff_t *offset) {
    // if (inode == NULL || buf == NULL || offset == NULL) {
    //     return -1;
    // }
    // if (*offset >= sizeof(struct system_time)) {
    //     *offset = 0; // 重置偏移量
    // }

    // size_t bytes_to_read = size;
    // if (*offset + bytes_to_read > sizeof(struct system_time)) {
    //     bytes_to_read = sizeof(struct system_time) - *offset; // 调整读取大小
    // }

    // uart_puts((char *)buf);
    // *offset += bytes_to_read;

    // return bytes_to_read;
// }

// static struct file_ops uart_file_ops = {
    // .open = uart_open,
    // .read = uart_read,
    // .write = uart_write,
// };

static int uart_probe(struct platform_device *pdev) {
    struct device_node *node = of_find_node_by_compatible("imx6ull,uart");
    if (!node) {
        return -1;
    }

    uint32_t *reg = of_read_u32_array(node, "reg", 2);
    uart_base = (virt_addr_t)ioremap(reg[0], reg[1]);

    // uart_reg_init();

    // dev_t devnr = 2;
    // register_chrdev(devnr, "uart", &uart_file_ops);
    // if (lookup("/uart") == NULL)
    //     mknod("/uart", S_IFCHR | 0644, devnr);

    // int virq = platform_get_irq(pdev, 0);
    // irq_register(virq, uart0_iqr, "uart0_irq",NULL);

    console_register(putc);
    // irq_enable(virq);
    return 0;
}

static void uart_remove() {
    // unregister_chrdev(2, "/uart");
    // if (lookup("/uart")) {
    // }
    iounmap(uart_base, sizeof(UART_Type));
    uart_base = 0;
}

static struct of_device_id uart_of_match[] = {
    {.compatible = "imx6ull,uart",},
    {/* sentinel */}
};

static struct platform_driver uart_driver = {
    .name = "uart_driver",
    .probe = uart_probe,
    .remove = uart_remove,
    .of_match_table = uart_of_match,
};

module_platform_driver(uart_driver);
