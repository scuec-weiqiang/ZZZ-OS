/*******************************************************************************************
 * @FilePath: /ZZZ/kernel/sched.c
 * @Description  :  
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @Date         : 2025-04-16 21:02:39
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2025-04-20 16:59:37
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#include "printf.h"
#include "page.h"
#include "sched.h"
#include "task.h"

scheduler_t scheduler;

static uint64_t         _sched_check(uint64_t now_time);
static reg_t            _setup_next_task(uint64_t now_time);
static enum sched_state _get_sched_state(uint64_t now_time);

void sched_init()
{
    INIT_LIST_HEAD(&scheduler.running_queue);
    INIT_LIST_HEAD(&scheduler.blocked_queue);
    scheduler.current_task = NULL_PTR;
    // scheduler.task_num = 0;
}

/*******************************************************************************************
 * @brief: 
 * @param {reg_t} epc
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
reg_t sched(reg_t epc,uint64_t now_time)
{
    if(!list_empty(&need_add_task)) // 如果有任务需要添加到运行队列
    {
        list_splice(&need_add_task,&scheduler.running_queue) ;//把任务添加到运行队列
        INIT_LIST_HEAD(&need_add_task);
    }

    switch (_get_sched_state(now_time))
     {
        case SCHED_IDLE:    
            // printf("sched_idle\n");
            return epc; 
            break;

        case SCHED_FIRST:  
            // printf("sched_first\n");
            return _setup_next_task(now_time);
            break;

        case SCHED_RUNNING: 
            // printf("sched_running\n");
            return epc; 
            break;

        case SCHED_SWITCHING:
            // printf("sched_switching\n");
            list_mov_tail(&scheduler.running_queue,scheduler.running_queue.next);//将到期任务移动到链表尾部
            return _setup_next_task(now_time);
            break;
        default:
            return epc;
     }
}

/*******************************************************************************************
 * @brief: 
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
static reg_t _setup_next_task(uint64_t now_time)
{
    scheduler.current_task = list_entry(scheduler.running_queue.next,tcb_t,node);//获取下一个将要执行的任务
    scheduler.current_task->expire_time = now_time  + scheduler.current_task->time_slice;//设置将要执行的任务的到期时间
    mscratch_w((reg_t)&scheduler.current_task->reg_context);//设置mscratch寄存器，指向将要执行的任务的上下文
    return scheduler.current_task->reg_context.mepc;//返回将要执行的任务的入口地址
}

/*******************************************************************************************
 * @brief: 
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
static uint64_t _sched_check(uint64_t now_time)
{
    return now_time >= scheduler.current_task->expire_time?1:0;
}


/*******************************************************************************************
 * @brief: 
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
static enum sched_state _get_sched_state(uint64_t now_time)
{
    if (IS_NULL_PTR(scheduler.current_task))//如果没有任务在执行
    {
        if (list_empty(&scheduler.running_queue)) //当前没有任务在执行，且运行队列为空，表明系统空闲
        {
            return SCHED_IDLE;
        }
        else //当前没有任务在执行，但是运行队列不为空，表明这是调度器第一次进入任务
        {
            return SCHED_FIRST;
        }
    }

    //如果有任务在执行,检查当前任务时间是否到期
    if (!_sched_check(now_time))
    {
        return SCHED_RUNNING; // 时间片未用完，继续执行当前任务
    }
    else
    {
        return SCHED_SWITCHING; //任务时间到期，需要切换任务
    }

}


