/**
 * @FilePath: /ZZZ-OS/lib/rand.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 22:42:57
 * @LastEditTime: 2025-11-25 22:46:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
// 不依赖标准库的随机数生成器（线性同余生成器 LCG）
// 仅使用C语言基础语法，无外部库依赖

// 静态全局变量存储当前种子（保证每次调用延续随机序列）
static long long seed = 1;  // 默认种子，可通过my_srand修改

/**
 * @brief 初始化随机数种子
 * @param s 自定义种子值（通常用时间戳，若无需时间戳可直接传固定值）
 */
void srand(long long s) {
    seed = s;
}

/**
 * @brief 生成一个[0, 2147483647]范围内的伪随机数
 * @return 32位有符号整数范围内的随机数
 */
int rand(void) {
    // LCG经典参数：a=1103515245, c=12345, m=2^31
    seed = (1103515245LL * seed + 12345LL) % 2147483648LL;
    // 返回非负整数（转换为32位有符号int）
    return (int)(seed & 0x7FFFFFFF);
}

/**
 * @brief 生成指定范围[min, max]的随机数
 * @param min 范围最小值
 * @param max 范围最大值
 * @return [min, max]范围内的随机数
 */
int rand_range(int min, int max) {
    if (min > max) {  // 处理min > max的异常情况
        int temp = min;
        min = max;
        max = temp;
    }
    // 取模缩放到目标范围
    return (rand() % (max - min + 1)) + min;
}
