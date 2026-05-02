/**
 * @FilePath: /ZZZ-OS/lib/string.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-09 02:40:09
 * @LastEditTime: 2025-10-10 00:17:08
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */

#include <os/string.h>
#include <os/kmalloc.h>

/**
 * @brief 将内存区域填充为指定值
 * @param dest 目标内存起始地址
 * @param ch   填充的字节值
 * @param size 填充的字节数
 * @return 指向 dest 的指针
 */
void *memset(void *dest, int ch, size_t size)
{
    char *d = dest;
    for (size_t i = 0; i < size; i++)
    {
        d[i] = (char)ch;
    }
    return dest;
}

int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
    const unsigned char *p1 = ptr1;
    const unsigned char *p2 = ptr2;
    for (size_t i = 0; i < num; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

/**
 * @brief 从源内存复制数据到目标内存
 * @param dest 目标内存起始地址
 * @param src  源内存起始地址
 * @param size 复制的字节数
 * @return 指向 dest 的指针
 */
void *memcpy(void *dest, const void *src, size_t size)
{
    char *d = dest;
    const char *s = src;
    for (size_t i = 0; i < size; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

void *memcpy32(void *dest, const void *src, size_t size)
{
    u32 *d = dest;
    const u32 *s = src;
    size_t words = size / 4;
    for (size_t i = 0; i < words; i++)
        d[i] = s[i];
    return dest;
}

int strcpy(char *dest, const char *src)
{
    size_t i;
    for (i = 0; src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    dest[i] = '\0'; // 确保目标字符串以'\0'结尾
    return 0;
}

int strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++)
    {
        dest[i] = src[i];
    }
    for (; i < n; i++)
    {
        dest[i] = '\0'; // 填充剩余部分为'\0'
    }
    return 0;
}

int strlen(const char *s)
{
    const char *p = s;
    while (*p != '\0')
    {
        p++;
    }
    return p - s; // 返回字符串长度
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2; // 返回差值
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0')
        {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0; // 前n个字符相等
}

char *strdup(const char *s)
{
    if (s == NULL)
        return NULL;

    size_t len = strlen(s) + 1; // 计算长度（含结束符）
    char *dup = kmalloc(len);    // 分配内存

    if (dup != NULL)
    {
        strcpy(dup, s); // 复制字符串
    }

    return dup;
}

/**
 * @brief 将路径字符串分割成多个token
 *
 * 该函数用于将给定的路径字符串分割成多个token，每个token表示路径中的一个部分。
 * 如果传入的字符串为NULL，则继续从上次分割的位置开始分割。
 *
 * @param str 要分割的路径字符串，如果为NULL则继续上次分割的位置
 *
 * @return 返回下一个token的起始位置，如果没有更多token则返回NULL。
 */
char *strtok(char *str, const char *delim) {
    static char *cur_token = NULL; // 用于保存下一个token的起始位置
    static char *end = NULL;       // 用于保存字符串结束位置

    if (str != NULL)
    {
        cur_token = str; // 保存当前token的起始位置
        end = str;
        // 如果str不为NULL，说明是第一次调用
        for (int i = 0; str[i] != '\0'; i++)
        {
            if (str[i] == delim[0]) // 找到路径分隔符
            {
                str[i] = '\0'; // 将分隔符替换为字符串结束符
            }
            end++; // 更新end指针，直到指向字符串的末尾
        }
    }
    else
    {
        // 如果str为NULL，说明是后续调用
        while (*cur_token != '\0') // 这时cur_token指向上一个token的开头，需要跳过上一段token指向下一个/0分隔符
        {
            if (cur_token == end) // 如果
            {
                cur_token = NULL;
                break; // 如果已经到达字符串末尾，返回NULL
            }
            cur_token++;
        }
    }

    while (*cur_token =='\0') // 这时cur_token已经不再指向上一个token的内容，但是有可能上个token结尾有多个/0分隔符，需要跳过这些分隔符指向下一个token开头
    {
        // 注意，一定要先判断是否到达字符串末尾，否则不会及时跳出循环，会越界报错
        if (cur_token == end)
        {
            cur_token = NULL;
            break; // 如果已经到达字符串末尾，返回NULL
        }
        cur_token++;
    }

    return cur_token;
}
// 其他函数：memmove、memcmp、strlen 等