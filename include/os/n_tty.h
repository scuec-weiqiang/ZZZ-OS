/**
 * @FilePath     : /ZZZ-OS/include/os/n_tty.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-15 16:10:45
 * @LastEditTime : 2026-07-15 16:10:46
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef __OS_N_TTY_H
#define __OS_N_TTY_H

#include <os/types.h>
#include <os/ringbuffer.h>
#include <os/spinlock.h>

#define N_TTY_BUF_SIZE 4096

struct n_tty_data {
    struct ringbuffer read_buf;
    char read_buf_data[N_TTY_BUF_SIZE];
    spinlock_t read_lock;

    size_t canon_head;
    size_t line_start;

    unsigned int line_count;
};

int n_tty_init(void);

#endif
