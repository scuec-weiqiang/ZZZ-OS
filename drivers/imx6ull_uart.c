/**
 * @FilePath     : /ZZZ-OS/drivers/imx6ull_uart.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-12 00:25:01
 * @LastEditTime : 2026-03-26 00:04:55
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

// #include <fs/chrdev.h>
// #include <fs/vfs.h>
#include <os/console.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/string.h>
#include <os/irqreturn.h>
#include <os/irq.h>
#include <os/printk.h>
#include <os/platform_device.h>

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

#define UART ((UART_Type*)uart_base)
// #define UART ((UART_Type*)0x02020000)

void uart_disable() {
	UART->UCR1 &= ~(1<<0);	
}


void uart_enable() {
	UART->UCR1 |= (1<<0);	
}

#define UCR1_UARTEN   (1U << 0)
#define UCR1_RRDYEN   (1U << 9)   // 接收就绪中断使能
#define UCR2_SRST     (1U << 0)
#define UCR2_RXEN     (1U << 1)
#define UCR2_TXEN     (1U << 2)

static void uart_enable_rx_irq() {
    UART->UCR2 |= UCR2_SRST | UCR2_RXEN | UCR2_TXEN;
    UART->UCR1 |= UCR1_UARTEN | UCR1_RRDYEN;
}

void uart_reg_init() {
    uart_disable();
    UART->UCR1 = 0;		/* 先清除UCR1寄存器 */
	
	/*
     * 设置UART的UCR1寄存器，关闭自动波特率
     * bit14: 0 关闭自动波特率检测,我们自己设置波特率
	 */
	UART->UCR1 &= ~(1<<14);
	
	/*
     * 设置UART的UCR2寄存器，设置内容包括字长，停止位，校验模式，关闭RTS硬件流控
     * bit14: 1 忽略RTS引脚
	 * bit8: 0 关闭奇偶校验
     * bit6: 0 1位停止位
 	 * bit5: 1 8位数据位
 	 * bit2: 1 打开发送
 	 * bit1: 1 打开接收
	 */
	UART->UCR2 |= (1<<14) | (1<<5) | (1<<2) | (1<<1);

	/*
     * UART1的UCR3寄存器
     * bit2: 1 必须设置为1！参考IMX6ULL参考手册3624页
	 */
	UART->UCR3 |= 1<<2; 
	
	/*
	 * 设置波特率
	 * 波特率计算公式:Baud Rate = Ref Freq / (16 * (UBMR + 1)/(UBIR+1)) 
	 * 如果要设置波特率为115200，那么可以使用如下参数:
	 * Ref Freq = 80M 也就是寄存器UFCR的bit9:7=101, 表示1分频
	 * UBMR = 3124
 	 * UBIR =  71
 	 * 因此波特率= 80000000/(16 * (3124+1)/(71+1))=80000000/(16 * 3125/72) = (80000000*72) / (16*3125) = 115200
	 */
	// UART->UFCR = 5<<7; //ref freq等于ipg_clk/1=80Mhz
	// UART->UBIR = 71;
	// UART->UBMR = 3124;
    uart_enable();
}

static void putc (char c) {
    while ((UART->UTS >> 4) & 1) {
    }
    UART->UTXD = c;
}

void uart_putc(char c) {
    while ((UART->UTS >> 4) & 1) {
    }
    UART->UTXD = c;
}

static char getc(void) {
    while((UART->USR2 & 0x1) == 0);/* 等待接收完成 */
	return UART->URXD;				/* 返回接收到的数据 */
}

void puts(char *s) {
    while (*s) {
        uart_putc(*s++);
        if (*s == '\n') {
            uart_putc('\r');
        }
    }
}

irqreturn_t uart_iqr(int virq, void *dev_id) {
    char a = getc();
    printk("%c",a);
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

    uart_base = platform_ioremap_resource(pdev, 0);

    uart_reg_init();

    // 打开串口中断
    uart_enable_rx_irq();

    int virq = platform_get_irq(pdev, 0);
    printk("uart irq: %d\n", virq);
    irq_request(virq, uart_iqr, "uart0_irq",NULL);
    irq_enable(virq);

    console_register(putc);
    
    return 0;
}

static int uart_remove(struct platform_device *pdev) {

    iounmap(uart_base, sizeof(UART_Type));
    uart_base = 0;
    return 0;
}

static struct of_device_id uart_of_match[] = {
    {.compatible = "imx6ull,uart",},
    {/* sentinel */}
};

static struct platform_driver uart_driver = {
    .name = "uart_driver",
    .probe = uart_probe,
    .remove = uart_remove,
    .driver = {
        .of_match_table = uart_of_match,
    }
};

module_platform_driver(uart_driver);
