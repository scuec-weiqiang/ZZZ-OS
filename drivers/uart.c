/**
 * @FilePath: /vboot/uart.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:10:59
 * @LastEditTime: 2025-09-17 20:20:50
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "platform.h"

struct uart_reg
{
   u8 RHR_THR_DLL;
   u8 IER_DLM;
   u8 FCR_ISR;
   u8 LCR;
   u8 MCR;
   u8 LSR;
   u8 MSR;
   u8 SPR;
};

#define UART0     (*(volatile struct uart_reg*)(UART_BASE))

#define UART_TX_IDLE (1<<5)
#define UART_RX_IDLE (1<<0)

#define WAIT_FOR_TRANS_READY(uartx)    while(0==(uartx.LSR & UART_TX_IDLE )) 
#define WAIT_FOR_RECEIVE_READY(uartx)  while(0==(uartx.LSR & UART_RX_IDLE))

void uart_init()
{
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