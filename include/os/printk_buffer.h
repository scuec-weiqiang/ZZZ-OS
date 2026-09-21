/**
 * @FilePath     : /ZZZ-OS/include/os/printk_buffer.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-19 16:44:00
 * @LastEditTime : 2026-09-19 16:44:01
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef _OS_PRINTK_BUFFER_H
#define _OS_PRINTK_BUFFER_H

#include <os/types.h>

#define PRINTK_TEXT_MAX 1023

struct printk_cursor {
    u64 seq;
    u32 idx;
};

struct printk_record {
    u64 ts_nsec;
    u16 text_len;
    u8 level;
    bool newline;

    char text[PRINTK_TEXT_MAX + 1];
};

// 将读游标初始化到当前最早的有效记录
void printk_cursor_init(struct printk_cursor *cursor);

//读取并复制一条记录，同时推进 cursor。返回 1：读到记录；0：当前没有新记录。
int printk_read_record(struct printk_cursor *cursor,struct printk_record *record);


#endif // _OS_PRINTK_BUFFER_H