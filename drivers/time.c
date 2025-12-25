/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/drivers/time.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-29 00:50:51
 * @LastEditTime: 2025-11-11 23:52:11
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <drivers/time.h>
#include <os/bswap.h>
#include <os/types.h>
#include <asm/platform.h>
#include <fs/vfs.h>
#include <fs/vfs_types.h>
#include <fs/chrdev.h>
#include <os/string.h>
#include <os/of.h>
#include <os/printk.h>
#include <os/module.h>
#include <os/mm.h>
#include <os/driver_model.h>
#include <drivers/core/driver.h>

// 固定地址的时间缓冲区
static struct system_time *system_time = NULL;

// 定义每月的天数（非闰年）
static const char days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// 判断是否为闰年
static int is_leap_year(uint32_t year) {
    if (year % 4 != 0)
        return 0;
    if (year % 100 != 0)
        return 1;
    if (year % 400 != 0)
        return 0;
    return 1;
}

// 计算从1970年到指定年份的总天数
static uint32_t days_since_epoch(uint32_t year, uint32_t month, uint32_t day) {
    uint32_t days = 0;

    // 累加完整年份的天数
    for (uint32_t y = 1970; y < year; y++) {
        days += is_leap_year(y) ? 366 : 365;
    }

    // 累加当月之前的月份天数
    for (uint32_t m = 1; m < month; m++) {
        days += days_in_month[m - 1];
        // 闰年2月加1天
        if (m == 2 && is_leap_year(year)) {
            days += 1;
        }
    }

    // 加当月天数（减1是因为day从1开始）
    days += (day - 1);

    return days;
}

void adjust_timezone(struct system_time *t, enum UTC utc) {
    int hour = (int)t->hour - utc;
    
    // 处理小时为负的情况（向前调整日期）
    while (hour < 0) {
        hour += 24;
        // 借一天
        t->day--;
        if (t->day == 0)
        {
            t->month--;
            if (t->month == 0)
            {
                t->month = 12;
                t->year -= 1;
            }
            // 获取当前月份的天数并设置
            t->day = days_since_epoch(t->year, t->month, 0); // 假设0表示获取当月天数
        }
    }

    // 处理小时超过24的情况（向后调整日期）
    while (hour >= 24)
    {
        hour -= 24;
        // 进一天
        t->day++;
        // 获取当前月份的最大天数
        int max_days = days_since_epoch(t->year, t->month, 0); // 假设0表示获取当月天数
        
        if (t->day > max_days)
        {
            t->day = 1;
            t->month++;
            if (t->month > 12)
            {
                t->month = 1;
                t->year += 1;
            }
        }
    }

    t->hour = (uint32_t)hour;
}

// 将system_time_t转换为Unix时间戳（自1970-01-01 00:00:00 UTC以来的秒数）
uint32_t system_time_to_unix_timestamp(const struct system_time *t) {
    // 基本校验
    if (t->year < 1970 || t->month < 1 || t->month > 12 ||
        t->day < 1 || t->day > 31 || t->hour > 23 ||
        t->minute > 59 || t->second > 59)
    {
        return 0; // 无效时间返回0
    }
    // 计算总秒数 = 天数×86400 + 小时×3600 + 分钟×60 + 秒
    uint32_t days = days_since_epoch(t->year, t->month, t->day);
    return days * 86400 + t->hour * 3600 + t->minute * 60 + t->second;
}

// 读取当前时间的函数
void get_current_time(struct system_time *t) {
    // 简单的内存读取操作
    *t = *system_time;
}

uint32_t get_current_unix_timestamp(enum UTC utc) {
    struct system_time t;
    if (system_time == NULL) {
        return 0;
    }
    get_current_time(&t);
    adjust_timezone(&t, utc);
    return system_time_to_unix_timestamp(&t);
}


static int timestamp_open(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t timestamp_read(struct inode *inode, void *buf, size_t size, loff_t *offset) {
    if (inode == NULL || buf == NULL || offset == NULL)
    {
        return -1; // 错误处理
    }

    if (*offset >= sizeof(struct system_time))
    {
        *offset = 0; // 重置偏移量
    }

    size_t bytes_to_read = size;
    if (*offset + bytes_to_read > sizeof(struct system_time))
    {
        bytes_to_read = sizeof(struct system_time) - *offset; // 调整读取大小
    }

    memcpy32(buf, (uint8_t *)system_time + *offset, bytes_to_read);
    *offset += bytes_to_read;

    return bytes_to_read;
}

static struct file_ops timestamp_file_ops = {
    .open = timestamp_open,
    .read = timestamp_read,
    .write = NULL
};


static int timestamp_probe(struct platform_device *pdev) {
    struct device_node *node =  of_find_node_by_compatible("wq,time");
    if (!node) {
        printk("time_init: can not find compatible wq,time\n");
        return -1;
    }

    uint32_t *reg = of_read_u32_array(node,"reg", 2);
    
    system_time = (struct system_time*)ioremap(reg[0], reg[1]);
    
    dev_t devnr = 1; 
    register_chrdev(devnr,"time", &timestamp_file_ops);
    if(lookup("/time") == NULL)
        mknod("/time", S_IFCHR|0644, devnr);
    printk("timestamp init success\n");
    return 0;
}

static void timestamp_remove(struct platform_device *pdev) {
    unregister_chrdev(1, "time");
    if (lookup("/time")) {
    }
    system_time = NULL;
}

static struct platform_driver timestamp_driver = {
    .name = "time_driver",
    .probe = timestamp_probe,
    .remove = timestamp_remove,
    .of_match_table = (const struct of_device_id[]) {
        { .compatible = "wq,time" },
        { /* sentinel */ }
    },
};

module_platform_driver(timestamp_driver);



