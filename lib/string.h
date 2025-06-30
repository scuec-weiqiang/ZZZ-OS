/**
 * @FilePath: /ZZZ/lib/string.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-09 02:40:04
 * @LastEditTime: 2025-06-29 14:36:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _STRING_H
#define _STRING_H

#include "types.h"  

extern void* memset(void *dest, int ch, size_t size);
extern void* memcpy(void *dest, const void *src, size_t size);
extern int strcmp(const char *s1, const char *s2);
extern int strcpy(char *dest, const char *src);
extern int strncpy(char *dest, const char *src, size_t n);
extern int strlen(const char *s);
#endif