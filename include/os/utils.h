/**
 * @FilePath: /vboot/lib/os/utils.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 23:13:38
 * @LastEditTime: 2025-09-17 23:17:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef UTILS_H
#define UTILS_H

unsigned int next_power_of_two(unsigned int n);
int is_power_of_two(unsigned int n); 

unsigned int div_u32(unsigned int num, unsigned int div_num);
unsigned int mod_u32(unsigned int num, unsigned int div_num);
unsigned int divmod_u32(unsigned int num, unsigned int div_num, unsigned int *rem);

unsigned long long div_u64(unsigned long long num, unsigned int div_num);
unsigned long long mod_u64(unsigned long long num, unsigned int div_num);
unsigned long long divmod_u64(unsigned long long num, unsigned int div_num, unsigned int *rem);

#endif