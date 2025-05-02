/**
 * @FilePath: /ZZZ/kernel/sched.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-01 02:29:14
 * @LastEditTime: 2025-05-02 16:25:03
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef SCHED_H
#define SCHED_H

#include "list.h"
#include "riscv.h"
#include "task.h"
#include "platform.h"

typedef enum sched_state { 
    SCHED_IDLE,
    SCHED_CONTINUE,
    SCHED_SWITCHING 
}sched_state_t;

typedef struct{ 
    // uint64_t task_num;
    list_t ready_queue;
    list_t wait_queue; 
    tcb_t *current_task;
}scheduler_t;

extern scheduler_t scheduler[MAX_HARTS];

extern void sched_init(hart_id_t hart_id);
extern reg_t sched(reg_t epc,uint64_t now_time,hart_id_t hart_id);
#endif


// #ifndef SCHEDULER_H
// #define SCHEDULER_H

// #include "task.h"
// enum sched_state { SCHED_IDLE, SCHED_FIRST, SCHED_RUNNING, SCHED_SWITCHING };

// typedef struct sched_interface {
//     // 初始化调度器
//     void (*init)(struct sched_class* self);
//     // 调度任务
//     reg_t (*pick_next)(struct sched_class* self, reg_t epc, uint64_t now_time);
//     // 添加任务
//     void (*add_queue)(struct sched_class* self, tcb_t* task);
//     // 删除任务
//     void (*del_queue)(struct sched_class* self, tcb_t* task);
//     // 获取当前任务
//     tcb_t* (*get_current)(struct sched_class* self);
// } sched_i;

// void scheduler_init(void);
// void schedule(void);
// // 注册调度类
// void sched_register_class(sched_policy_t policy, sched_i *cls);
// #endif


// //xxxx.h
// struct led_device;

// struct led_ops {
// int (*toggle)(struct led_device *);
// };

// #define to_led_device(_d) container_of(_d, struct led_device, dev)
// //xxxx.c
// static int id = 0;
// static struct led_device led_devices[10] = {0};

// struct led_device {
//     char name[32]; 
//     int id;
//     struct led_ops *ops;
// };

// void *alloc_led_device(const char *name, const struct led_ops *ops, int priv_size)
// {
//     struct led_device *led_dev = malloc(sizeof(struct led_device) +priv_size);
//     led_dev->ops = ops;
//     strcpy(led_dev->name, name);
//     led_dev->id = id++;
//     return led_dev - sizeof(*led_dev);
// }
// void toggle_led(struct led_device *dev)
// {
//     dev->ops->toggle(dev);
// }
// int led_device_register(struct led_device *dev)
// {
//     led_devices[dev->id] = dev;
//     return dev->id;
// }

// struct led_device *get_led_dev_byid(int id)
// {
//     return led_devices[id];
// }