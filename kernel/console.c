/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/kernel/console.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-11 20:15:50
 * @LastEditTime: 2025-11-14 01:57:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/compiler_attributes.h>

#define _LOG_BUF_SIZE 4096

static char log_buf[_LOG_BUF_SIZE] __aligned(8);
static size_t log_head = 0;
static size_t log_tail = 0;

static void (*fn_putc)(char c) = NULL; // 由外部注册

static int ring_is_empty(void) {
    return log_head == log_tail;
}

static int ring_is_full(void) {
    return ((log_head + 1) % _LOG_BUF_SIZE) == log_tail;
}

static void ring_putc(char c) {
    if (ring_is_full()) {
        log_tail = (log_tail + 1) % _LOG_BUF_SIZE;
    }

    log_buf[log_head] = c;
    log_head = (log_head + 1) % _LOG_BUF_SIZE;
}

static int ring_getc(void) {
    if (ring_is_empty()) {
        return -1;
    }
    char c = log_buf[log_tail];
    log_tail = (log_tail + 1) % _LOG_BUF_SIZE;
    return c;
}

void console_register(void (*fn)(char c)) {
    if (!fn || fn_putc) {
        return;
    }
    fn_putc = fn;
}

void console_puts(char *str) {
    int i = 0;
    while(str[i]) {
        ring_putc(str[i]);
        i++;
    }
}

void console_flush(void) {
    if (!fn_putc) {
        return;
    }

    int c = 0;
    c = (int)ring_getc();
    while (c != -1) {
        fn_putc((char)c);
        c = (int)ring_getc();
    } 
}
