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

extern uintptr_t _text_start;
extern uintptr_t _text_end;
extern uintptr_t _text_size;
extern uintptr_t _trap_start;
extern uintptr_t _trap_end;
extern uintptr_t _trap_size;
extern uintptr_t _rodata_start;
extern uintptr_t _rodata_end;
extern uintptr_t _rodata_size;
extern uintptr_t _data_start;
extern uintptr_t _data_end;
extern uintptr_t _data_size;
extern uintptr_t _bss_start;
extern uintptr_t _bss_end;
extern uintptr_t _bss_size;
extern uintptr_t _stack_start;
extern uintptr_t _stack_end;
extern uintptr_t _stack_size;
extern uintptr_t _heap_start;
extern uintptr_t _heap_end;
extern uintptr_t _heap_size;
extern uintptr_t _systimer_ctx[5];

extern void maddr_def_init();
extern void zero_bss();

#endif