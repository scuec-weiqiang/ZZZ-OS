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
#include "page_alloc.h"
#include "sched.h"
#include "task.h"
#include "maddr_def.h"
#include "platform.h"

scheduler_t scheduler[MAX_HARTS_NUM];


__SELF uint64_t         _check_expire(uint64_t now_time,uint64_t expire_time);
__SELF tcb_t*           _get_current_task(hart_id_t hart_id);
__SELF tcb_t*           _get_next_task(hart_id_t hart_id);
__SELF reg_t            _setup_task(uint64_t now_time,hart_id_t hart_id,tcb_t* task);
__SELF sched_state_t    _get_sched_state(uint64_t now_time,tcb_t* current_task);

/*******************************************************************************************
 * @brief: 
 * @return {*}
*******************************************************************************************/
void sched_init(hart_id_t hart_id)
{
    INIT_LIST_HEAD(&scheduler[hart_id].ready_queue);
    INIT_LIST_HEAD(&scheduler[hart_id].wait_queue);
    scheduler[hart_id].current_task = NULL;
    // scheduler[hart_id].task_num = 0;
}

/**
 * @brief 调度函数，根据当前时间和调度状态决定下一个执行的任务
 *
 * @param epc 当前任务的入口地址
 * @param now_time 当前时间
 *
 * @return 下一个执行的任务的入口地址
 *
 * 如果存在需要添加到运行队列的任务，则将其添加到运行队列。
 * 根据当前时间和调度状态决定下一个执行的任务，并返回其入口地址。
 * 调度状态包括：
 * - SCHED_IDLE：空闲状态，不执行任何任务
 * - SCHED_FIRST：第一次调度，设置下一个任务的上下文并返回任务入口地址
 * - SCHED_CONTINUE：继续执行当前任务，返回当前任务的入口地址
 * - SCHED_SWITCHING：切换任务，将到期任务移动到链表尾部，设置下一个任务的上下文并返回任务入口地址
 */
reg_t sched(reg_t epc,uint64_t now_time,hart_id_t hart_id)
{
    reg_t ret = epc;

    if(!list_empty(&need_add_task[hart_id])) // 如果有任务需要添加到运行队列
    {
        list_splice(&need_add_task[hart_id],&scheduler[hart_id].ready_queue) ;//把任务添加到运行队列
        INIT_LIST_HEAD(&need_add_task[hart_id]);
    }

    tcb_t* current_task = _get_current_task(hart_id); //获取当前任务
    scheduler[hart_id].current_task = current_task; 
    
    tcb_t* del_task;

    if(current_task->status == TASK_ZOMBIE) //如果当前任务是僵尸状态，则将其从链表中删除
    {
        list_del(&current_task->node);
        page_free(current_task);
        current_task = NULL;
    }
    
    sched_state_t sched_state = _get_sched_state(now_time,current_task);

    switch (sched_state)
    {
        case SCHED_IDLE:    
            printf("core %d sched_idle\n",hart_id);
            ret = _setup_task(now_time,hart_id,NULL);
            break;

        case SCHED_CONTINUE:
            break;

        case SCHED_SWITCHING:
            // printf("sched_switching\n");
            ret = _setup_task(now_time,hart_id,_get_next_task(hart_id));//设置下一个任务的上下文并返回任务入口地址
            list_mov_tail(&scheduler[hart_id].ready_queue,&current_task->node);//将到期任务移动到链表尾部,但还没有更新current_task
            break;

        default:
            break;
    }
    return ret;
}

__SELF  tcb_t* _get_current_task(hart_id_t hart_id)
{
    if(list_empty(&scheduler[hart_id].ready_queue))
    {
        return NULL;
    }
    else
    {
        return list_entry(scheduler[hart_id].ready_queue.next,tcb_t,node); //获取下一个任务
    }
}

__SELF tcb_t* _get_next_task(hart_id_t hart_id)
{
    if(list_empty(&scheduler[hart_id].ready_queue))
    {
        return NULL;
    }
    else
    {
        return list_entry(scheduler[hart_id].ready_queue.next->next,tcb_t,node);;
    }
}

/**
 * @brief 设置任务上下文
 *
 * 根据给定的参数设置任务上下文。
 *
 * @param now_time 当前时间戳
 * @param hart_id 硬件线程ID
 * @param task 指向任务控制块的指针
 *
 * @return 返回机器异常程序计数器（mepc）的值
 *
 * 如果 task 为 NULL，则使用 kernel_reg_ctx_start 和 hart_id 计算内核上下文地址，
 * 并将该地址写入 mscratch 寄存器，最后返回该上下文的 mepc 值。
 *
 * 如果 task 不为 NULL，则更新任务的到期时间，并将任务的上下文地址写入 mscratch 寄存器，
 * 最后返回该任务的 mepc 值。
 */
__SELF reg_t _setup_task(uint64_t now_time,hart_id_t hart_id,tcb_t* task)
{
    // if(task == NULL)//
    // {
    //     reg_t* kernel_context = (_kernel_reg_ctx_start+hart_id*sizeof(reg_context_t));
    //     mscratch_w((reg_t*)kernel_context);
    //     return ((reg_context_t*)kernel_context)->mepc;
    // }
    // else
    // {
    //     task->expire_time = now_time  + task->time_slice;
    //     mscratch_w((reg_t)&task->reg_context);
    //     return task->reg_context.mepc;
    // }
}

/*******************************************************************************************
 * @brief: 
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
__SELF __INLINE uint64_t _check_expire(uint64_t now_time,uint64_t expire_time)
{
    return now_time >= expire_time?1:0;
}

/*******************************************************************************************
 * @brief: 
 * @param {uint64_t} now_time
 * @return {*}
*******************************************************************************************/
__SELF enum sched_state _get_sched_state(uint64_t now_time,tcb_t* current_task)
{
    if(current_task == NULL)
    {
        return SCHED_IDLE;
    }
    else 
    {
        // if(current_task->expire_time == 0)
        // {
        //     return  SCHED_FIRST;
        // }
        // else
        // {
            if(_check_expire(now_time,current_task->expire_time))
            {
                return  SCHED_SWITCHING;
            }
            else
            {
                return  SCHED_CONTINUE;
            }
        // }
    }

}
