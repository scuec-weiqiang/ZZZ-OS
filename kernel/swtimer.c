/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-12-04 19:04:33
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-12-08 23:05:56
 * @FilePath: /my_code/source/swtimer.c
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#include "os/types.h"
#include "os/malloc.h"
#include "os/list.h"
#include "asm/systimer.h"
#include "os/printk.h"
#include "os/sched.h"

#define SWTIMER_ON (1)
#define SWTIMER_OFF (0)
#define SWTIMER_DISTORY (-1)
#define SWTIMER_TIMEOUT (2)

struct swtimer
{
    uint64_t timer_id;
    uint64_t period; 
    char status;//-1:销毁，0:暂停，1:开启
    uint64_t tick;
    uint64_t mode;
    void (*timer_task)();
    struct list swtimer_node;    
};

struct swtimer *swtimer_head = NULL;

/***************************************************************
 * @description: 
 * @param {struct swtimer} *swtimer_d [in/out]:  
 * @return {*}
***************************************************************/
void swtimer_distory(struct swtimer *swtimer_d)
{
    list_del(&swtimer_d->swtimer_node);
    free(swtimer_d);
}

/***************************************************************
 * @description: 检查当前定时器是否超时
 * @param {struct swtimer} *swtimer_currrent [in/out]:  
 * @return {*}
***************************************************************/
static void _check_timeout(struct swtimer **swtimer_currrent)
{
    if(systick(0) >= (*swtimer_currrent)->tick
    &&(*swtimer_currrent)->status == SWTIMER_ON)
    {
        (*swtimer_currrent)->tick =  (*swtimer_currrent)->period + systick(0);
        (*swtimer_currrent)->timer_task();
        switch ( (*swtimer_currrent)->mode)
        {
            case 0: 
            break;

            case 1:
                // (*swtimer_currrent)->status = SWTIMER_DISTORY;//标记为需要销毁
                swtimer_distory((*swtimer_currrent));
            break;

            default:
                (*swtimer_currrent)->mode--;
            break;
        }
    } 
}


/***************************************************************
 * @description: 检查用户设定的定时器任务是否可以执行，如果满足条件就执行
 * @return {*}
***************************************************************/
void swtimer_check()
{
    struct swtimer *swtimer_currrent = NULL;
    // struct swtimer *next = NULL;
    if(NULL != swtimer_head)
    {   
        list_for_each_entry(swtimer_currrent,&swtimer_head->swtimer_node,struct swtimer,swtimer_node) 
        {   
            _check_timeout(&swtimer_currrent);
        }
    }
}
 
/***************************************************************
 * @description: 创建一个软件定时器
 * @param void (*timer_task)() [in]: 定时器任务
 * @param {uint64_t} period [in]:  执行的周期长短，以硬件定时器的tick为单位
 * @param {uint8_t} mode [in]:  执行次数，达到次数后会被销毁
 *                  mode == 0 --> 无限次
 * @return {struct swtimer*} 返回创建的定时器指针
***************************************************************/
struct swtimer* swtimer_create(void (*timer_task)(),uint64_t period,uint8_t mode)
{
    struct swtimer *new_timer = page_alloc(1);        
    if((struct swtimer*)NULL == new_timer) return NULL;
 
    switch ((uint64_t)swtimer_head)
    {
        case (uint64_t)NULL:
            static uint64_t id = 0;
            swtimer_head = page_alloc(1); 
            INIT_LIST_HEAD(&swtimer_head->swtimer_node);
        default:
            new_timer->timer_task = timer_task;
            new_timer->period = period;
            new_timer->mode = mode; 
            new_timer->timer_id = id;
            new_timer->tick =  period + systick(0);
            new_timer->status = SWTIMER_ON;
            list_add_tail(&swtimer_head->swtimer_node,&new_timer->swtimer_node);
            id++;
        break;
    }
    return new_timer;
}
