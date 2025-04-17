#include "uart.h"

void uart_init()
{
    UART0.IER_DLM = 0x00;//关闭中断

    UART0.LCR |= (1<<7);//允许访问除数锁寄存器
    UART0.IER_DLM = 0x00;//波特率38.4k
    UART0.RHR_THR_DLL = 0x03;
    UART0.LCR &= ~(1<<7);//禁止访问除数锁寄存器

    UART0.LCR |= (0x03<<0);//设置传输字长为8位
    UART0.LCR &= ~(1<<2);//停止位 1位

    uint32_t a = UART0.IER_DLM;
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
