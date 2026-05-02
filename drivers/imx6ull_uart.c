/**
 * @FilePath     : /ZZZ-OS/drivers/imx6ull_uart.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2026-03-12 00:25:01
 * @LastEditTime : 2026-03-26 00:04:55
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <fs/cdev.h>
// #include <fs/vfs.h>
#include <os/console.h>
#include <os/mm.h>
#include <os/of.h>
#include <os/string.h>
#include <os/irqreturn.h>
#include <os/irq.h>
#include <os/printk.h>
#include <os/platform_device.h>
#include <os/spinlock.h>
#include <os/wait.h>
#include <os/sched.h>   


#define __IO volatile
#define __I  volatile const

typedef struct {
    __I  u32 URXD;                              /**< UART Receiver Register, offset: 0x0 */
         u8 RESERVED_0[60];
    __IO u32 UTXD;                              /**< UART Transmitter Register, offset: 0x40 */
         u8 RESERVED_1[60];
    __IO u32 UCR1;                              /**< UART Control Register 1, offset: 0x80 */
    __IO u32 UCR2;                              /**< UART Control Register 2, offset: 0x84 */
    __IO u32 UCR3;                              /**< UART Control Register 3, offset: 0x88 */
    __IO u32 UCR4;                              /**< UART Control Register 4, offset: 0x8C */
    __IO u32 UFCR;                              /**< UART FIFO Control Register, offset: 0x90 */
    __IO u32 USR1;                              /**< UART Status Register 1, offset: 0x94 */
    __IO u32 USR2;                              /**< UART Status Register 2, offset: 0x98 */
    __IO u32 UESC;                              /**< UART Escape Character Register, offset: 0x9C */
    __IO u32 UTIM;                              /**< UART Escape Timer Register, offset: 0xA0 */
    __IO u32 UBIR;                              /**< UART BRM Incremental Register, offset: 0xA4 */
    __IO u32 UBMR;                              /**< UART BRM Modulator Register, offset: 0xA8 */
    __I  u32 UBRC;                              /**< UART Baud Rate Count Register, offset: 0xAC */
    __IO u32 ONEMS;                             /**< UART One Millisecond Register, offset: 0xB0 */
    __IO u32 UTS;                               /**< UART Test Register, offset: 0xB4 */
    __IO u32 UMCR;                              /**< UART RS-485 Mode Control Register, offset: 0xB8 */
  } UART_Type;

#if 1
  #define _UART ((UART_Type*)0x02020000)
  void _putc(char c) {
      while ((_UART->UTS >> 4) & 1) {
      }
      _UART->UTXD = c;
  }
  
  void _puts(char *s) {
      while (*s) {
          _putc(*s++);
          if (*s == '\n') {
              _putc('\r');
          }
      }
  }
#endif

phys_addr_t uart_base = 0;

#define UART ((UART_Type*)uart_base)

#define UART_RX_BUF_SIZE 256

struct uart_rx_buffer {
    spinlock_t lock;
    unsigned int head;
    unsigned int tail;
    char data[UART_RX_BUF_SIZE];
};

static struct uart_rx_buffer uart_rxbuf;
static struct wait_queue_head uart_wait_queue = WAIT_QUEUE_INIT(uart_wait_queue);
static struct irq_deferred_work uart_rx_deferred_work;

static void uart_disable() {
	UART->UCR1 &= ~(1<<0);	
}

static void uart_enable() {
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

static void uart_reg_init() {
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

static int uart_rx_ready(void) {
    return (UART->USR2 & 0x1) != 0;
}

static char uart_hw_getc(void) {
    return (char)(UART->URXD & 0xff);
}

static char getc(void) {
    while (!uart_rx_ready()) {
    }
	return uart_hw_getc();
}

static int uart_rxbuf_is_empty(void) {
    return uart_rxbuf.head == uart_rxbuf.tail;
}

static int uart_rxbuf_is_full(void) {
    return ((uart_rxbuf.head + 1) % UART_RX_BUF_SIZE) == uart_rxbuf.tail;
}

static void uart_rxbuf_push(char ch) {
    unsigned long flags;

    flags = spin_lock_irqsave(&uart_rxbuf.lock);
    if (!uart_rxbuf_is_full()) {
        uart_rxbuf.data[uart_rxbuf.head] = ch;
        uart_rxbuf.head = (uart_rxbuf.head + 1) % UART_RX_BUF_SIZE;
    }
    spin_unlock_irqrestore(&uart_rxbuf.lock, flags);
}

static int uart_rxbuf_pop(char *ch) {
    unsigned long flags;
    int ok = 0;

    flags = spin_lock_irqsave(&uart_rxbuf.lock);
    if (!uart_rxbuf_is_empty()) {
        *ch = uart_rxbuf.data[uart_rxbuf.tail];
        uart_rxbuf.tail = (uart_rxbuf.tail + 1) % UART_RX_BUF_SIZE;
        ok = 1;
    }
    spin_unlock_irqrestore(&uart_rxbuf.lock, flags);

    return ok;
}

static void uart_rx_deferred(void *arg)
{
    (void)arg;

    if (!wait_queue_empty(&uart_wait_queue) && !uart_rxbuf_is_empty()) {
        wake_up_one(&uart_wait_queue);
    }
}

irqreturn_t uart_iqr(int virq, void *dev_id) {
    (void)virq;
    (void)dev_id;

    while (uart_rx_ready()) {
        char ch = uart_hw_getc();
        uart_rxbuf_push(ch);
    }

    if (!wait_queue_empty(&uart_wait_queue)&& !uart_rxbuf_is_empty()) {
        irq_deferred_work_queue(&uart_rx_deferred_work);
    }
   

    return IRQ_HANDLED;
}

static int uart_open(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t uart_write(struct file *fp, const char *buf, size_t size, loff_t *offset) {
    size_t written = 0;

    if (fp == NULL || buf == NULL || offset == NULL) {
        return -1;
    }

    while (written < size) {
        putc(buf[written]);
        written++;
    }

    *offset += written;
    return (ssize_t)written;
}

static ssize_t uart_read(struct file *fp, char *buf, size_t size, loff_t *offset) {
    size_t read = 0;

    if (fp == NULL || buf == NULL || offset == NULL) {
        return -1;
    }

    while (read < size) {
        char ch;

        if(!uart_rx_ready() && uart_rxbuf_is_empty()) {
            sleep_on(&uart_wait_queue);
        }

        if (uart_rxbuf_pop(&ch)) {
            buf[read++] = ch;
            continue;
        }

        if (uart_rx_ready()) {
            buf[read++] = uart_hw_getc();
            continue;
        }

        if (read > 0) {
            break;
        }

        buf[read++] = getc();
    }

    *offset += read;
    return (ssize_t)read;
}

static struct file_operations uart_file_ops = {
    .open = uart_open,
    .read = uart_read,
    .write = uart_write,
};


static int uart_probe(struct platform_device *pdev) {
    struct device_node *node = of_find_node_by_compatible("imx6ull,uart");
    if (!node) {
        return -1;
    }

    uart_base = platform_ioremap_resource(pdev, 0);
    dprintk("uart base = %xu\n",uart_base);
    spin_lock_init(&uart_rxbuf.lock);
    uart_rxbuf.head = 0;
    uart_rxbuf.tail = 0;
    irq_deferred_work_register(&uart_rx_deferred_work, uart_rx_deferred, NULL);
    uart_reg_init();

    // 打开串口中断
    uart_enable_rx_irq();

    int virq = platform_get_irq(pdev, 0);
    printk("uart irq: %d\n", virq);
    irq_request(virq, uart_iqr, "uart0_irq",NULL);
    irq_enable(virq);

    console_register(putc);

    dev_t devnr;
    alloc_chrdev_region(&devnr, 1);
    
    cdev_register("uart0", devnr, &uart_file_ops, NULL);

    
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
