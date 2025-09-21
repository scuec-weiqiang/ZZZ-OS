/**
 * @FilePath: /ZZZ/kernel/task.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 21:02:39
 * @LastEditTime: 2025-05-09 02:44:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _TASK_H
#define _TASK_H

#include "types.h"
#include "riscv.h"
#include "list.h"
#include "platform.h"

#define TASK_STACK_SIZE 1024 //每个任务分配1k的栈大小

typedef enum task_status{
    TASK_READY,
    TASK_WAIT,
    TASK_ZOMBIE
}task_status_t;

typedef enum task_priority{
    TASK_PRIO_IDLE,
    TASK_PRIO_NORMAL,
    TASK_PRIO_HIGH,
}task_priority_t;

typedef struct task_ctrl_block
{   
    u64 id;
    u64 expire_time;
    u64 time_slice;
    task_priority_t priority;
    task_status_t status;
    u8  __attribute__((aligned(16))) task_stack[TASK_STACK_SIZE];
    void (*task)(void *param);
    struct list node;

}tcb_t;

typedef tcb_t* task_handle_t;

//这是个中介，作为链表头，需要加入调度的任务会挂载到这个链表上，调度器会从这个链表拆取任务合并到调度器自己的链表中
extern struct list need_add_task[MAX_HARTS_NUM];

extern void task_init();
extern task_handle_t  task_create(enum hart_id hart_id,void (*task)(void *param),u64 time_slice,u8 priority);
extern void task_delete(task_handle_t del_task);

extern void task_delay(volatile int count);

#endif
// #ifndef TASK_H
// #define TASK_H

// #include "types.h"

// #include "riscv.h"
// #include "list.h"

// // 调度策略类型
// typedef enum {
//     SCHED_POLICY_RR,    // 时间片轮转调度
//     SCHED_POLICY_IDLE   // 空闲调度
// } sched_policy_t;

// /**
//  * @brief: 调度基类
// */
// typedef struct sched_entity{
//     sched_policy_t policy;
//     struct list sched_entity_node;
// }sched_entity_t;

// /**
//  * @brief: 空闲调度
// */
// typedef struct idle_sched_entity{
//     sched_entity_t base;
// } idle_sched_entity_t;

// /**
//  * @brief: 时间片轮转调度
// */
// typedef struct rr_sched_entity{
//     sched_entity_t base;
//     u8 priority;
//     u64 remaining;
//     u64 time_slice;
// } rr_sched_entity_t;

// /**
//  * @brief: 任务状态枚举
// */
// typedef enum task_status{
//     TASK_RUNNING,
//     TASK_READY,
//     TASK_BLOCKED,
//     TASK_ZOMBIE
// }task_status_t;

// #define TASK_STACK_SIZE 1024 //每个任务分配1k的栈大小

// typedef struct task_ctrl_block
// {   
//     u64 id;
//     task_status_t status;
//     struct reg_context reg_context;
//     u8  __attribute__((aligned(16))) task_stack[TASK_STACK_SIZE];
//     void (*entry)(void *param);
//     union {
//         rr_sched_entity_t rr_entity;
//         idle_sched_entity_t idle_entity;
//     } se;
// }tcb_t;

// typedef tcb_t* task_handle_t;

// extern task_handle_t task_create(sched_policy_t policy, void (*entry)(void*), u64 time_slice, u8 priority);
// extern void task_distory(task_handle_t task_handle);
// extern void task_delay(volatile int count);
// // extern void task_set_priority(task_handle_t task, u8 priority); // 新增优先级设置接口
// // extern task_status_t task_get_status(task_handle_t task);
// #endif