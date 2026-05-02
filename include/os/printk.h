/**
 * @FilePath: /vboot/os/printk.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 19:40:02
 * @LastEditTime: 2025-09-17 21:08:01
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef PRINTF_H
#define PRINTF_H

#define GREEN(msg)          "\033[32m" msg "\033[0m"
#define RED(msg)            "\033[31m" msg "\033[0m"
#define BLUE(msg)           "\033[34m" msg "\033[0m"
#define YELLOW(msg)         "\033[33m" msg "\033[0m"
#define CYAN(msg)           "\033[36m" msg "\033[0m"
#define WHITE(msg)          "\033[37m" msg "\033[0m"
#define BOLD(msg)           "\033[1m" msg "\033[0m"
#define ITALIC(msg)         "\033[3m" msg "\033[0m"
#define UNDERLINE(msg)      "\033[4m" msg "\033[0m"
#define BLINK(msg)          "\033[5m" msg "\033[0m"
#define INVERSE(msg)        "\033[7m" msg "\033[0m"
#define HIDDEN(msg)         "\033[8m" msg "\033[0m"
#define STRIKE(msg)         "\033[9m" msg "\033[0m"

extern int printk(const char* s, ...);
extern void panic(const char* s, ...);


#define PRINTK_DEBUG

#ifdef PRINTK_DEBUG
#define dprintk(fmt, ...) \
    printk(YELLOW("[DBG] %s:%d:%s: ") fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define dprintk(fmt, ...) do { } while (0)
#endif

#define here dprintk(RED("here: %s:%d\n"), __FILE__, __LINE__)

#endif
