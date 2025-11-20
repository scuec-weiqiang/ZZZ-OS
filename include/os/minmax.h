/**
 * @FilePath: /ZZZ-OS/include/os/minmax.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-21 00:37:37
 * @LastEditTime: 2025-11-21 00:38:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_MINMAX_H
#define __KERNEL_MINMAX_H

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define umin(a,b) (((a)<(b))?(a):(b))
#define umax(a,b) (((a)>(b))?(a):(b))

#endif