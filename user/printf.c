#include <stdarg.h>
#include "types.h"
#include "usys.h"

#define __PBUFF_SIZE 4096

// 对齐，避免 64 位 RISC-V 栈错位
static char print_buff[__PBUFF_SIZE] __attribute__((aligned(8)));

/**
 * 数字转字符串（支持 10/16/2 进制）
 */
int num2char(char* str, unsigned int pos, unsigned long long num, int decimal)
{
    unsigned int digit = 1;
    for (unsigned long long temp = num; temp /= decimal; digit++);

    if (str) {
        if (decimal == 16) {
            str[pos++] = '0'; str[pos++] = 'x';
        } else if (decimal == 2) {
            str[pos++] = '0'; str[pos++] = 'b';
        }
        for (int i = digit - 1; i >= 0; i--) {
            int rem = num % decimal;
            str[pos + i] = (rem < 10) ? ('0' + rem) : ('a' + rem - 10);
            num /= decimal;
        }
    }
    return digit + ((decimal == 10) ? 0 : 2);
}

/**
 * 格式化核心
 */
int _vsprint(char* out_buff, const char *fmt, va_list vl)
{
    int pos = 0, format = 0, decimal = 0;

    for (; *fmt; fmt++) {
        if (format) {
            switch (*fmt) {
                case 'x': decimal = 16; goto DEC;
                case 'b': decimal = 2; goto DEC;
                case 'd': 
                DEC: {
                    long long num = va_arg(vl, long long);
                    if (decimal == 0) decimal = 10;
                    if (num < 0 && out_buff && *(fmt+1)!='u') {
                        out_buff[pos++] = '-';
                        num = -num;
                    }
                    pos += num2char(out_buff, pos, num, decimal);
                    decimal = 0;
                    format = 0;
                    break;
                }
                case 'c': {
                    int c = va_arg(vl, int);
                    if (out_buff) out_buff[pos] = (char)c;
                    pos++;
                    format = 0;
                    break;
                }
                case 's': {
                    const char *s = va_arg(vl, const char*);
                    while (*s) {
                        if (out_buff) out_buff[pos] = *s;
                        pos++;
                        s++;
                    }
                    format = 0;
                    break;
                }
                default:
                    format = 0;
                    break;
            }
        } else if (*fmt == '%') {
            format = 1;
        } else {
            if (out_buff) out_buff[pos] = *fmt;
            pos++;
        }
    }

    if (out_buff) out_buff[pos] = '\0';
    return pos;
}

/**
 * vprintf: 先计算长度，再格式化
 */
int _vprint(const char *fmt, va_list vl)
{
    va_list vl_copy;
    va_copy(vl_copy, vl);

    int n = _vsprint(NULL, fmt, vl_copy);
    va_end(vl_copy);
    if (n >= __PBUFF_SIZE) {
        print("printk overflow!\n", 20);
        while (1) {} // 死机
    }

    _vsprint(print_buff, fmt, vl);
    va_end(vl_copy);

    print(print_buff,n);
    return n;
}


/**
 * printk: 有锁
 */
int printf(const char *fmt, ...)
{

    va_list vl;
    va_start(vl, fmt);
    int n = _vprint(fmt, vl);
    va_end(vl);

    return n;
}

/**
 * panic: 不可重入 printf
 */
void panic(const char *fmt, ...)
{
    print("panic: ",1);

    va_list vl;
    va_start(vl, fmt);
    int n = _vsprint(print_buff, fmt, vl);
    va_end(vl);

    print("panic: ",7);
    print(print_buff,n);
    print("\n",1);

    while (1) {}
}
