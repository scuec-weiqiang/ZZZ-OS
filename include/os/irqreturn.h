/**
 * @FilePath: /ZZZ-OS/include/os/irqreturn.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-11-01 00:48:33
 * @LastEditTime: 2025-11-01 00:48:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef __KERNEL_IRQRETURN_H__
#define __KERNEL_IRQRETURN_H__

typedef enum irqreturn {
	IRQ_NONE		= (0 << 0),
	IRQ_HANDLED		= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
}irqreturn_t;

#endif // __KERNEL_IRQRETURN_H__