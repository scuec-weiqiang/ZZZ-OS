/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-10-12 16:19:07
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-27 20:01:05
 * @FilePath: /my_code/include/platform.h
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef PALTFORM_H
#define PALTFORM_H

#define MAXNUM_CPU 2

#define LENGTH_RAM 128*1024*1024

enum HART_ID{
    HART_0,
    HART_1
};

#define SYS_CLOCK_FREQ 10000000

#endif