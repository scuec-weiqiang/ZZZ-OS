/**
 * @FilePath: /ZZZ/lib/string.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-09 02:40:09
 * @LastEditTime: 2025-08-26 18:24:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#include "string.h"
#include "page_alloc.h"

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


/**
 * @brief 比较两个字符串
 *
 * 比较两个字符串s1和s2，返回其字典序的差值。
 *
 * @param s1 第一个字符串指针
 * @param s2 第二个字符串指针
 *
 * @return 如果s1小于s2，则返回负值；
 *         如果s1等于s2，则返回0；
 *         如果s1大于s2，则返回正值。
 */
int strcpy(char *dest, const char *src) {
    size_t i;
    for (i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0'; // 确保目标字符串以'\0'结尾
    return 0;
}

int strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0'; // 填充剩余部分为'\0'
    }
    return 0;
}

int strlen(const char *s) {
    const char *p = s;
    while (*p != '\0') {
        p++;
    }
    return p - s; // 返回字符串长度
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2; // 返回差值
}

char *strdup(const char *s) {
    if (s == NULL) return NULL;
    
    size_t len = strlen(s) + 1;  // 计算长度（含结束符）
    char *dup = malloc(len);     // 分配内存
    
    if (dup != NULL) {
        strcpy(dup, s);          // 复制字符串
    }
    
    return dup;
}
// 其他函数：memmove、memcmp、strlen 等