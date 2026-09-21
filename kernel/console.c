/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/kernel/console.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-11 20:15:50
 * @LastEditTime: 2025-11-14 01:57:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/console.h>
#include <os/spinlock.h>
#include <os/errno.h>
#include <os/string.h>
#include <os/printk_buffer.h>
#include <os/printk.h>

static struct console console_list = {
    .name = "null",
    .write = NULL,
    .private = NULL,
    .cursor = {0, 0},
    .next = NULL,
};

// 这个锁是保护console_owned和console_pending的，只保护 谁负责输出 这几个状态
// console_owned表示当前是否有线程正在负责输出日志，console_pending表示是否有新的日志需要输出
static SPINLOCK_DEFINE(console_state_lock);
static bool console_owned;
static bool console_pending;

static struct printk_record console_record; 

/* 调用前必须先获取锁 */
struct console *console_get_locked(const char *name) 
{
    struct console *con = &console_list;
    while (con) {
        if (strcmp(con->name, name) == 0) {
            return con;
        }
        con = con->next;
    }
    return NULL;
}

int console_register(struct console *con) 
{
    if (!con || !con->name || !con->write) {
        return -EINVAL;
    }

    struct console *existing = console_get_locked(con->name);
    if (existing) {
        return -EEXIST;
    }
    printk_cursor_init(&con->cursor);
    struct console *next = console_list.next; 
    console_list.next = con;
    con->next = next;

    console_kick();
    return 0;
}

static void console_emit_record(struct console *con, struct printk_record *record) 
{
    if (!con || !record) {
        return;
    }
    // 如果日志级别低于控制台日志级别，则不输出
    if (record->level >= console_loglevel) {
        return;
    }

    char timestamp_buf[32];
    u64 ts_nsec = record->ts_nsec;
    size_t timestamp_len;

    if (record->newline) {
        record->text[record->text_len++] = '\n';
    }

    timestamp_len = scnprintf(timestamp_buf,
                                    sizeof(timestamp_buf),
                                    "[%5llu.%06llu] ",
                                    ts_nsec / 1000000000ULL,
                                    (ts_nsec % 1000000000ULL) / 1000ULL);
    
                
    if (record->level < console_loglevel) {
        timestamp_len = scnprintf(timestamp_buf,
                                    sizeof(timestamp_buf),
                                    "[%5llu.%06llu] ",
                                    ts_nsec / 1000000000ULL,
                                    (ts_nsec % 1000000000ULL) / 1000ULL);
        con->write(con, timestamp_buf, timestamp_len);
        con->write(con, record->text, record->text_len);
    }
    
}

static void console_drain(void)
{
    struct console *con;
    // char timestamp_buf[32];

    for (con = console_list.next; con; con = con->next) {
        while (printk_read_record(&con->cursor,
                                  &console_record) > 0) {
            if (console_record.level >= console_loglevel)
                continue;

            console_emit_record(con, &console_record);
        }
    }
}

void console_kick(void) 
{
    // 拿到锁后查看当前是否有线程正在负责输出日志，
    // 如果有就设置console_pending为true，表示有新的日志需要输出
    unsigned long flags;
    spin_lock_irqsave(&console_state_lock, &flags);
    if (console_owned) {
        console_pending = true;
        spin_unlock_irqrestore(&console_state_lock, flags);
        return;
    }

    // 没有线程负责输出日志，当前线程就可以负责输出日志
    console_owned = true;
    spin_unlock_irqrestore(&console_state_lock, flags);
    while (1) {
        console_drain();

        // 输出完当前日志后，检查是否有新的日志需要输出
        spin_lock_irqsave(&console_state_lock, &flags);
        if (!console_pending) {
            console_owned = false;
            spin_unlock_irqrestore(&console_state_lock, flags);
            break;
        }
        console_pending = false;
        spin_unlock_irqrestore(&console_state_lock, flags);
    }

}

void console_flush_sync(void) {

}