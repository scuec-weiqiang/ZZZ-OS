/**
 * @FilePath: /ZZZ-OS/lib/bswap.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-22 22:25:06
 * @LastEditTime: 2025-10-22 22:25:45
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef macros_lib_bswap_h
#define macros_lib_bswap_h

static inline unsigned int __bswapsi2(unsigned int x) {
    return (x << 24) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | (x >> 24);
}

// 可选：如果后续需要64位转换，也可以实现 __bswapdi2
static inline unsigned long long __bswapdi2(unsigned long long x) {
    return (x << 56) | ((x << 40) & 0x00FF000000000000) |
           ((x << 24) & 0x0000FF0000000000) | ((x << 8) & 0x000000FF00000000) |
           ((x >> 8) & 0x00000000FF000000) | ((x >> 24) & 0x0000000000FF0000) |
           ((x >> 40) & 0x000000000000FF00) | (x >> 56);
}

#endif