/**
 * @FilePath: /ZZZ/lib/time.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-29 00:50:51
 * @LastEditTime: 2025-08-29 22:49:22
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "time.h"

// 固定地址的时间缓冲区
volatile system_time_t *const system_time = (system_time_t*)0x50000000;

// 定义每月的天数（非闰年）
static const uint8_t days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};

// 判断是否为闰年
static int is_leap_year(uint32_t year) {
    if (year % 4 != 0) return 0;
    if (year % 100 != 0) return 1;
    if (year % 400 != 0) return 0;
    return 1;
}

// 计算从1970年到指定年份的总天数
static uint32_t days_since_epoch(uint32_t year, uint32_t month, uint32_t day) 
{
    uint32_t days = 0;
    
    // 累加完整年份的天数
    for (uint32_t y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }
    
    // 累加当月之前的月份天数
    for (uint32_t m = 1; m < month; m++) {
        days += days_in_month[m-1];
        // 闰年2月加1天
        if (m == 2 && is_leap_year(year)) {
            days += 1;
        }
    }
    
    // 加当月天数（减1是因为day从1开始）
    days += (day - 1);
    
    return days;
}

void adjust_timezone(system_time_t *t, UTC_t utc) {
    int hour = (int)t->hour - utc;

    while (hour < 0) {
        hour += 24;
        // 借一天
        if (--t->day == 0) {
            if (--t->month == 0) {
                t->month = 12;
                t->year -= 1;
            }
            t->day = days_since_epoch(t->year, t->month,t->day);
        }
    }

    while (hour >= 24) {
        hour -= 24;
        // 进一天
        if (++t->day > days_since_epoch(t->year, t->month, t->day)) {
            t->day = 1;
            if (++t->month > 12) {
                t->month = 1;
                t->year += 1;
            }
        }
    }

    t->hour = (uint32_t)hour;
}
// 将system_time_t转换为Unix时间戳（自1970-01-01 00:00:00 UTC以来的秒数）
uint32_t system_time_to_unix_timestamp(const system_time_t *t) {
    // 基本校验
    if (t->year < 1970 || t->month < 1 || t->month > 12 || 
        t->day < 1 || t->day > 31 || t->hour > 23 || 
        t->minute > 59 || t->second > 59) {
        return 0; // 无效时间返回0
    }
    
    // 计算总秒数 = 天数×86400 + 小时×3600 + 分钟×60 + 秒
    uint32_t days = days_since_epoch(t->year, t->month, t->day);
    return days * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}
    
// 读取当前时间的函数
void get_current_time(system_time_t *t) 
{
    // 简单的内存读取操作
    *t = *system_time;
}

uint32_t get_current_unix_timestamp(UTC_t utc) 
{
    system_time_t t;
    get_current_time(&t);
    adjust_timezone(&t, utc);
    return system_time_to_unix_timestamp(&t);
}

