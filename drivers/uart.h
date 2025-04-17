/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-10-26 16:38:03
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-13 00:22:28
 * @FilePath: /my_code/include/uart.h
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef UART_H
#define UART_H

#include "types.h"

typedef struct UART_REG
{
   uint8_t RHR_THR_DLL;
   uint8_t IER_DLM;
   uint8_t FCR_ISR;
   uint8_t LCR;
   uint8_t MCR;
   uint8_t LSR;
   uint8_t MSR;
   uint8_t SPR;
}UART_REG_t;

#define UART0     (*(volatile UART_REG_t*)(0x10000000))

#define UART_TX_IDLE (1<<5)
#define UART_RX_IDLE (1<<0)

#define WAIT_FOR_TRANS_READY(uartx)    while(0==(uartx.LSR & UART_TX_IDLE )) 
#define WAIT_FOR_RECEIVE_READY(uartx)  while(0==(uartx.LSR & UART_RX_IDLE))

extern void uart_init();
extern void uart_putc(char c);
extern void uart_puts(char *s);
extern char uart_getc();

#endif