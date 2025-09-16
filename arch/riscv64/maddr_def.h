/**
 * @FilePath: /ZZZ/arch/riscv64/maddr_def.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-16 21:29:36
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/*******************************************************************************************
 * @FilePath     : /ZZZ/arch/riscv64/maddr_def.h
 * @Description  :  这里定义了一些链接脚本中的内存地址，方便在其他文件中引用。
 * @Author       : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditors  : scuec_weiqiang scuec_weiqiang@qq.com
 * @LastEditTime : 2025-04-17 19:55:34
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*******************************************************************************************/
#ifndef __MADDR_DEF_H__
#define __MADDR_DEF_H__

extern char _text_start[], _text_end[], _text_size[];
extern char _trap_start[], _trap_end[], _trap_size[];
extern char _rodata_start[], _rodata_end[], _rodata_size[];
extern char _data_start[], _data_end[], _data_size[];
extern char _bss_start[], _bss_end[], _bss_size[];
extern char _stack_start[], _stack_end[], _stack_size[];
extern char _heap_start[], _heap_end[];
#define _heap_size (_heap_end - _heap_start)

extern char _systimer_ctx[];

#endif