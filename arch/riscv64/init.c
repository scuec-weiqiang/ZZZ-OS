/**
 * @FilePath: /ZZZ/kernel/init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 00:43:47
 * @LastEditTime: 2025-05-02 19:24:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "riscv.h"

void init()
{
    
    satp_w(0);

    // 将所有异常和中断委托给S模式处理
    medeleg_w(0xffff);
    mideleg_w(0xffff);
    
    
}