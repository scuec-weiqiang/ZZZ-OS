/**
 * @FilePath: /vboot/lib/utils.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 23:13:36
 * @LastEditTime: 2025-09-17 23:17:32
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/config.h>
#include <os/utils.h>

unsigned int next_power_of_two(unsigned int n) {
    if (n == 0) return 1;

    n--;                 // 先减 1，避免本身就是 2^k 时多算一倍
    n |= n >> 1;         // 把最高位右边所有位都置 1
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;        // 加 1 得到结果
}

int is_power_of_two(unsigned int n) 
{
    return (n != 0) && ((n & (n - 1)) == 0);
}

#if SYS_BITS == 64

unsigned int div_u32(unsigned int num, unsigned int div_u32) {
    return num / div_num;
}

unsigned int mod_u32(unsigned int num, unsigned int div_u32) {
    return num % div_num;
}

unsigned int divmod_u32(unsigned int num, unsigned int div_num, unsigned int *rem) {
    unsigned int ret =  num / div_num;
    if (rem) *rem = num % div_num;
    return ret;
}

unsigned long long divmod_u64(unsigned long long num, unsigned int div_num, unsigned int *rem) {
    unsigned long long ret =  num / div_num;
    if (rem) *rem = num % div_num;
    return ret;
}

#elif SYS_BITS == 32
unsigned int div_u32(unsigned int num, unsigned int div_num) {
    unsigned int q = 0;
    unsigned int r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((num >> i) & 1);
        if (r >= div_num) {
            r -= div_num;
            q |= (1U << i);
        }
    }
    return q;
}

unsigned int mod_u32(unsigned int num, unsigned int div_num) {
    unsigned int r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((num >> i) & 1);
        if (r >= div_num) {
            r -= div_num;
        }
    }
    return r;
}

unsigned int divmod_u32(unsigned int num, unsigned int div_num, unsigned int *rem) {
    unsigned int q = div_u32(num, div_num);
    if (rem) *rem = mod_u32(num, div_num);
    return q;
}


unsigned long long div_u64(unsigned long long num, unsigned int div_num) {
    unsigned long long q = 0;
    unsigned long long r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((num >> i) & 1);
        if (r >= div_num) {
            r -= div_num;
            q |= (1ULL << i);
        }
    }
    return q;
}

unsigned long long mod_u64(unsigned long long num, unsigned int div_num) {
    unsigned long long r = 0;
    for (int i = 63; i >= 0; i--) {
        r = (r << 1) | ((num >> i) & 1);
        if (r >= div_num) {
            r -= div_num;
        }
    }
    return r;
}

// 返回 num / div，rem 保存余数
unsigned long long divmod_u64(unsigned long long num, unsigned int div_num, unsigned int *rem) {
    unsigned long long q = div_u64(num, div_num);
    if (rem) *rem = mod_u64(num, div_num);
    return q;
}
#endif
