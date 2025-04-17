/*******************************************************************************************
 * @FilePath     : /ZZZ/arch/riscv64/hwtimer.c
 * @Description  : 内核定时器的实现文件。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-17 01:40:22
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/

#include "types.h"
#include "clint.h" 

uint64_t hwtimer_tick = 0;

/*******************************************************************************************
 * @brief        : 
 * @param         {uint64_t} hart_id:
 * @param         {uint64_t} value:
 * @return        {*}
*******************************************************************************************/
void hwtimer_load(uint64_t hart_id,uint64_t value)
{   
    uint64_t temp = __clint_mtime_get();
    temp += value;
    __clint_mtimecmp_set(0,temp);

}
