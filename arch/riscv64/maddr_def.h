/**
 * @FilePath: /ZZZ/arch/riscv64/maddr_def.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-07 19:18:08
 * @LastEditTime: 2025-09-17 17:41:02
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

extern uintptr_t text_start;
extern uintptr_t text_end;
extern uintptr_t text_size;
extern uintptr_t trap_start;
extern uintptr_t trap_end;
extern uintptr_t trap_size;
extern uintptr_t rodata_start;
extern uintptr_t rodata_end;
extern uintptr_t rodata_size;
extern uintptr_t data_start;
extern uintptr_t data_end;
extern uintptr_t data_size;
extern uintptr_t bss_start;
extern uintptr_t bss_end;
extern uintptr_t bss_size;
extern uintptr_t stack_start;
extern uintptr_t stack_end;
extern uintptr_t stack_size;
extern uintptr_t heap_start;
extern uintptr_t heap_end;
extern uintptr_t heap_size;
extern uintptr_t core_num;

extern void maddr_def_init();
extern void zero_bss();

#endif