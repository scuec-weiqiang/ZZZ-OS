/**
 * @FilePath: /ZZZ/lib/utils.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-22 17:43:39
 * @LastEditTime: 2025-08-22 17:43:54
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
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