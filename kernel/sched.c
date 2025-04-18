/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-10-31 12:22:31
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-12-08 21:17:05
 * @FilePath: /my_code/source/sched.c
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#include "printf.h"
#include "page.h"
#include "sched.h"
#include "task.h"
#include "hwtimer.h"

scheduler_t scheduler;

void sched_init()
{
    INIT_LIST_HEAD(&scheduler.running_queue);
    INIT_LIST_HEAD(&scheduler.blocked_queue);
    scheduler.current_task = NULL_PTR;
    // scheduler.task_num = 0;
}

/***************************************************************
 * @description: 
 * @param {volatile int} count [in/out]:  
 * @return {*}
***************************************************************/

reg_t sched(reg_t epc)
{

    if(!list_empty(&need_add_task)) // 如果有任务需要添加到运行队列
    {
        list_splice(&need_add_task,&scheduler.running_queue) ;
        INIT_LIST_HEAD(&need_add_task);
    }

    if(!list_empty(&scheduler.running_queue))
    {

        scheduler.current_task = list_entry(scheduler.running_queue.next,tcb_t,node);
        hwtimer_reload_ms(scheduler.current_task->time_slice);
        mscratch_w(&scheduler.current_task->reg_context);

        if(IS_NULL_PTR(scheduler.current_task)) //第一次进入任务
        {
            return scheduler.current_task->reg_context.ra;
        }
        else
        {
            list_mov_tail(&scheduler.running_queue,scheduler.running_queue.next);
            return scheduler.current_task->reg_context.mepc;
        }
    }
    else
    {
        printf("time\n");
        hwtimer_reload_s(1);
        return epc;
    }
        
}




