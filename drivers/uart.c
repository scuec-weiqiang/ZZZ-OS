/**
 * @FilePath: /vboot/uart.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:10:59
 * @LastEditTime: 2025-09-17 20:20:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "platform.h"
#include "types.h"
#include "fdt.h"
#include "bswap.h"
#include "vfs.h"
#include "vfs_types.h"
#include "chrdev.h"
#include "string.h"
#include "platform.h"

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

uintptr_t uart_base= UART_BASE;
#define UART0     (*(volatile struct uart_reg*)(uart_base))

#define UART_TX_IDLE (1<<5)
#define UART_RX_IDLE (1<<0)

#define WAIT_FOR_TRANS_READY(uartx)    while(0==(uartx.LSR & UART_TX_IDLE )) 
#define WAIT_FOR_RECEIVE_READY(uartx)  while(0==(uartx.LSR & UART_RX_IDLE))

void uart_reg_init() {

    UART0.IER_DLM = 0x00;//关闭中断

    UART0.LCR |= (1<<7);//允许访问除数锁寄存器
    UART0.IER_DLM = 0x00;//波特率38.4k
    UART0.RHR_THR_DLL = 0x03;
    UART0.LCR &= ~(1<<7);//禁止访问除数锁寄存器

    UART0.LCR |= (0x03<<0);//设置传输字长为8位
    UART0.LCR &= ~(1<<2);//停止位 1位

    u8 a = UART0.IER_DLM;
    a |= 0x01;
    UART0.IER_DLM = a;//打开中断
}

void uart_putc(char c)
{
    WAIT_FOR_TRANS_READY(UART0);
    UART0.RHR_THR_DLL = c;
}

char uart_getc()
{
    WAIT_FOR_RECEIVE_READY(UART0);
    return UART0.RHR_THR_DLL;
    
}

void uart_puts(char *s)
{
    while(*s)
    {
        uart_putc(*s);
        s++;
    }
}

void uart0_iqr()
{   
    // char a = uart_getc();
    // printk("%c",a);
    // if('\r'==a)
    //     printk("\n");     
}

static int uart_open(struct inode *inode, struct file *file) {
    return 0;
}


static ssize_t uart_write(struct inode *inode, const void *buf, size_t size, loff_t *offset) {
    if (inode == NULL || buf == NULL || offset == NULL) {
        return -1;
    }
    if (*offset >= sizeof(struct system_time))
    {
        *offset = 0; // 重置偏移量
    }

    size_t bytes_to_read = size;
    if (*offset + bytes_to_read > sizeof(struct system_time))
    {
        bytes_to_read = sizeof(struct system_time) - *offset; // 调整读取大小
    }

    uart_puts((char*)buf);
    *offset += bytes_to_read;

    return bytes_to_read;   
}

static struct file_ops uart_file_ops = {
    .open = uart_open,
    // .read = uart_read,
    .write = uart_write,
};

int uart_init()
{
    struct fdt_node *node =  fdt_find_node_by_compatible("wq,uart");
    if (!node) {
        return -1;
    }

    u32 *reg = fdt_get_reg(node);
    
    uart_base = (uintptr_t)be32_to_cpu(*(unsigned int*)reg);
    
    dev_t devnr = 2; 
    register_chrdev(devnr,"uart", &uart_file_ops);
    if(lookup("/uart") == NULL)
        mknod("/uart", S_IFCHR|0644, devnr);
    return 0;
}

void uart_deinit() {
    unregister_chrdev(2, "/uart");
    if (lookup("/uart")) {
        
    }
    uart_base = NULL;
}


