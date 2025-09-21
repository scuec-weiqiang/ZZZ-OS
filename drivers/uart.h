/**
 * @FilePath: /ZZZ/drivers/uart.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:10:49
 * @LastEditTime: 2025-09-20 16:59:07
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef UART_H
#define UART_H

extern void uart_init();
extern void uart_putc(char c);
extern void uart_puts(char *s);
extern char uart_getc();
extern void uart0_iqr();

#endif