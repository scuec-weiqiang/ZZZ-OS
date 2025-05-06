/**
 * @FilePath: /ZZZ/user/user.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-17 18:55:35
 * @LastEditTime: 2025-05-02 19:23:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "task.h"
#include "printf.h"
#include "systimer.h"
#include "swtimer.h"
#include "color.h"

task_handle_t task0_handle;
task_handle_t task1_handle;

uint64_t t = 0;
void task0_timer()
{
    t++;
    printf("t = %d\n",t);
}

void task0()
{
    uint32_t a = 0;
    // swtimer_create(task0_timer,100,5);
    // uint32_t b = 1;
    while (1)
    {
        a++;        
        task_delay(1000);
        hart_id_t id = get_hart_id();
        if(id==0)
            printf(YELLOW("hart %d ---> task0 running  %d\r\n"),id,a);
        else
            printf(GREEN("hart %d ---> task0 running  %d\r\n"),id,a);
        // printf("in task0,sp = %x",sp_r());
        // printf("task0 running\r");
        // mhartid_r();
        // printf("hartid = %d\n",b);
    }
    
}

void task1()
{
    uint32_t a = 0;
    while (1)
    {
        a++;   
        if(a==5)
        {
            printf("task1 delete \n");
            task_delete(task1_handle);
        }
        task_delay(1000);    
        hart_id_t id = get_hart_id();
        if(id==0)
            printf(RED("hart %d ---> task1 running  %d\r\n"),id,a);
        else
            printf(BLUE("hart %d ---> task1 running  %d\r\n"),id,a);
        // printf("task1 running\r");
    }
}

void task2()
{
    uint32_t a = 0;
    while (1)
    {
        a++;       
        task_delay(200); 
        printf("task2 running  %d\r\n",a);
    }
}

void task3()
{
    uint32_t a = 0;
    while (1)
    {
        a++;        
        task_delay(200);
        printf("task3 running  %d\r\n",a);
    }
}

void __attribute((aligned(4))) os_main(hart_id_t hart_id)
{
    printf("hart%d init complete!\n",hart_id);
    task0_handle =  task_create(hart_id,task0,(hart_id+1)*40*tick_ms,0);
    task1_handle =  task_create(hart_id,task1,(hart_id+1)*30*tick_ms,0);
    
    // printf("create 2 tasks \n");
    // task_create(task2,300*tick_ms,0);
    // task_create(task3,100*tick_ms,0);
    while(1){} 

}