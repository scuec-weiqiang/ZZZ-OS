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

extern int printk(const char* s, ...);
extern void panic(const char* s, ...);

#endif