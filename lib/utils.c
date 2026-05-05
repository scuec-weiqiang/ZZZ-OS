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

unsigned int div_u32(unsigned int num, unsigned int div_num) {
    return num / div_num;
}

unsigned int mod_u32(unsigned int num, unsigned int div_num) {
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

unsigned long long div_u64(unsigned long long num, unsigned int div_num) {
    return divmod_u64(num, div_num, 0);
}

unsigned long long mod_u64(unsigned long long num, unsigned int div_num) {
    unsigned int rem = 0;

    divmod_u64(num, div_num, &rem);
    return rem;
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

static unsigned long long do_divmod_u64(unsigned long long num,
                                        unsigned int div_num,
                                        unsigned int *rem)
{
    unsigned int num_hi = (unsigned int)(num >> 32);
    unsigned int num_lo = (unsigned int)num;
    unsigned int q_hi = 0;
    unsigned int q_lo = 0;
    unsigned int r = 0;
    int i;

    if (div_num == 0) {
        if (rem) {
            *rem = 0;
        }
        return 0;
    }

    /*
     * Divide a 64-bit dividend by a 32-bit divisor using only 32-bit shifts.
     * This avoids the broken variable-width 64-bit shift sequence generated
     * for ARM32 in the previous bit-by-bit implementation.
     */
    for (i = 31; i >= 0; i--) {
        unsigned int tmp_hi = r >> 31;
        unsigned int tmp_lo = (r << 1) | ((num_hi >> i) & 1U);

        if (tmp_hi || tmp_lo >= div_num) {
            tmp_lo -= div_num;
            q_hi |= (1U << i);
        }
        r = tmp_lo;
    }

    for (i = 31; i >= 0; i--) {
        unsigned int tmp_hi = r >> 31;
        unsigned int tmp_lo = (r << 1) | ((num_lo >> i) & 1U);

        if (tmp_hi || tmp_lo >= div_num) {
            tmp_lo -= div_num;
            q_lo |= (1U << i);
        }
        r = tmp_lo;
    }

    if (rem) {
        *rem = r;
    }

    return ((unsigned long long)q_hi << 32) | q_lo;
}

unsigned long long div_u64(unsigned long long num, unsigned int div_num) {
    return do_divmod_u64(num, div_num, 0);
}

unsigned long long mod_u64(unsigned long long num, unsigned int div_num) {
    unsigned int rem = 0;

    do_divmod_u64(num, div_num, &rem);
    return rem;
}

// 返回 num / div，rem 保存余数
unsigned long long divmod_u64(unsigned long long num, unsigned int div_num, unsigned int *rem) {
    return do_divmod_u64(num, div_num, rem);
}
#endif
