/**
 * @FilePath: /ZZZ/kernel/swtimer.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-16 21:02:39
 * @LastEditTime: 2025-05-09 02:42:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-12-04 19:04:50
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-12-05 19:31:23
 * @FilePath: /my_code/include/swtimer.h
 * @Description: 
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved. 
***************************************************************/
#ifndef _SWTIMER_H
#define _SWTIMER_H

#include "types.h"

typedef struct swtimer swtimer_t;

extern swtimer_t *swtimer_head;

extern swtimer_t* swtimer_create(void (*timer_task)(),uint64_t set_time,uint8_t mode);
extern void swtimer_distory(swtimer_t *swtimer_d);
extern void swtimer_check();


#endif