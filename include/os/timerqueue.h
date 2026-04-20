/**
 * @FilePath: /ZZZ/kernel/swtimer.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 21:02:39
 * @LastEditTime: 2025-05-09 02:42:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef __OS_TIMERQUEUE_H
#define __OS_TIMERQUEUE_H

#include <os/types.h>

struct timer {
    uint64_t expires_ns;              // 到期时间，基于 monotonic
    uint64_t period_ns;   // 0 表示 one-shot
    int cpu; // 记录现在运行在哪个cpu上，方便调度器迁移
    int pinned; // 是否绑定到特定 CPU，-1 表示不绑定，其他值表示绑定的 CPU ID
    void (*callback)(struct timer *t, void *arg);
    void *arg;
    bool active;
    int heap_idx;    // 小根堆，方便删除
};

#define MAX_TIMERS 128

extern int timerqueue_init(void);
extern int timer_start(struct timer *t);
extern int timer_cancel(struct timer *t);
extern uint64_t timerqueue_next_deadline(void);
extern void timerqueue_run_expired(uint64_t now);
extern int timerqueue_empty(void);
extern void timerqueue_test(void);

#endif