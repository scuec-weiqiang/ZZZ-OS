/**
 * @FilePath: /ZZZ-OS/drivers/time.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 23:00:58
 * @LastEditTime: 2025-10-09 22:46:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef TIME_H
#define TIME_H

#include "types.h"

typedef struct timespec 
{
    uint64_t tv_sec;   // 秒数（自 Unix 纪元时间 1970-01-01 00:00:00 起）
    uint64_t tv_nsec;  // 纳秒数（0 ~ 999,999,999）
}timespec_t;

// 定义时间结构体
struct system_time
{
    uint32_t year;   // 年份
    uint32_t month;  // 月份 (1-12)
    uint32_t day;    // 日期 (1-31)
    uint32_t hour;   // 小时 (0-23)
    uint32_t minute; // 分钟 (0-59)
    uint32_t second; // 秒 (0-59)
    uint32_t usec;   // 微秒 (0-999999)
};

enum UTC
{
    UTC_1 = -1,
    UTC_2 = -2,
    UTC_3 = -3,
    UTC_4 = -4,
    UTC_5 = -5,
    UTC_6 = -6, 
    UTC_7 = -7,
    UTC_8 = -8,
    UTC_9 = -9,
    UTC_10 = -10,
    UTC_11 = -11,
    UTC_12 = -12,
    UTC0 = 0, // UTC+0
    UTC1 = 1, // UTC+1
    UTC2 = 2, // UTC+2
    UTC3 = 3, // UTC+3
    UTC4 = 4, // UTC+4
    UTC5 = 5, // UTC+5
    UTC6 = 6, // UTC+6
    UTC7 = 7, // UTC+7
    UTC8 = 8,  //
    UTC9 = 9,
    UTC10 = 10,
    UTC11 = 11,
    UTC12 = 12
};

extern void get_current_time(struct system_time *t);
extern uint32_t get_current_unix_timestamp(enum UTC utc); 

extern int timestamp_init();
extern uint32_t system_time_to_unix_timestamp(const struct system_time *t);

#endif