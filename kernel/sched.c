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

scheduler_t scheduler;



//此函数定义在switch.S文件中
extern void _switch(reg_context_t* next);

/***************************************************************
 * @description: 
 * @param {volatile int} count [in/out]:  
 * @return {*}
***************************************************************/
void task_delay(volatile int count)
{
	count *= 50000;
	while (count--);
}

void sched()
{
   if(!list_empty(&need_add_task))
   {
       return;
   }
}

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
// void switch_task()
// {
//     task_current = list_entry(task_current->node.next,tcb_t,node); 
//     __sw_save(&task_current->reg_context);
// }

void sched_init()
{
    scheduler.running_queue = NULL_PTR;
    scheduler.blocked_queue = NULL_PTR;
    mscratch_w(&scheduler.reg_context);

}

// p /x *task_ctrl_block

// t0 0x8000c000
// t1 0x8000d000

// x /1xw 0x8000c07c
//x /1xw 0x8000c07c




