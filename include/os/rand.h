/**
 * @FilePath: /ZZZ-OS/include/os/rand.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-25 22:44:31
 * @LastEditTime: 2025-11-25 22:45:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_RAND_H__
#define __KERNEL_RAND_H__

extern void srand(long long s);
extern int rand(void);
extern int rand_range(int min, int max);

#endif /* __KERNEL_RAND_H__ */