/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-11-12 23:18:55
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-27 23:20:09
 * @FilePath: /my_code/source/interrupt.c
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#include "plic.h"
#include "clint.h"

#include "riscv.h"

#include "types.h"

/***************************************************************
 * @description: 开启全局中断
 * @return {*}
***************************************************************/
void global_interrupt_enable()
{
    mstatus_w(mstatus_r()|0x08);
}

/***************************************************************
 * @description: 关闭全局中断
 * @return {*}
***************************************************************/
void global_interrupt_disable()
{
    mstatus_w(mstatus_r()&(~0x08));
}

/***************************************************************
 * @description: 开启内核tick定时器中断
 * @return {*}
***************************************************************/
void timer_interrupt_enable()
{
    mie_w(mie_r()|0x80);
}

/***************************************************************
 * @description: 关闭内核tick定时器中断
 * @return {*}
***************************************************************/
void timer_interrupt_disable()
{
    mie_w(mie_r()&(~0x80));
}

/***************************************************************
 * @description: 开启外部中断
 * @return {*}
***************************************************************/
void extern_interrupt_enable()
{
    mie_w(mie_r()|0x800);
}

/***************************************************************
 * @description: 关闭外部中断
 * @return {*}
***************************************************************/
void extern_interrupt_disable()
{
    mie_w(mie_r()&(~0x800));
}


/***************************************************************
 * @description: 外部中断设置
 * @param {uint32_t} hart [in]:  指定某一hartid
 * @param {uint32_t} iqrn [in]:  外部中断源的中断号
 * @param {uint32_t} priority [in]:  外部中断的优先级
 * @return {*}
***************************************************************/
void extern_interrupt_setting(uint32_t hart,uint32_t iqrn,uint32_t priority)
{ 
    __plic_priority_set(iqrn,priority);
    __plic_threshold_set(hart,0);
    __plic_interrupt_enable(hart,iqrn);
} 


/***************************************************************
 * @description: 开启soft中断
 * @return {*}
***************************************************************/
void soft_interrupt_enable()
{
    mie_w(mie_r()|0x08);
}

/***************************************************************
 * @description: 关闭soft中断
 * @return {*}
***************************************************************/
void soft_interrupt_disable()
{
    mie_w(mie_r()&(~0x08));
}