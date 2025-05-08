/**
 * @FilePath: /ZZZ/lib/string.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-09 02:40:09
 * @LastEditTime: 2025-05-09 02:40:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "string.h"

/**
 * @brief 将内存区域填充为指定值
 * @param dest 目标内存起始地址
 * @param ch   填充的字节值
 * @param size 填充的字节数
 * @return 指向 dest 的指针
 */
void* memset(void *dest, int ch, size_t size) {
    char *d = dest;
    for (size_t i = 0; i < size; i++) {
        d[i] = (char)ch;
    }
    return dest;
}

/**
 * @brief 从源内存复制数据到目标内存
 * @param dest 目标内存起始地址
 * @param src  源内存起始地址
 * @param size 复制的字节数
 * @return 指向 dest 的指针
 */
void* memcpy(void *dest, const void *src, size_t size) {
    char *d = dest;
    const char *s = src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
    return dest;
}

// 其他函数：memmove、memcmp、strlen 等