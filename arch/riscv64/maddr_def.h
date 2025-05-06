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

extern char _text_start[], _text_end[];
extern char _rodata_start[], _rodata_end[];
extern char _data_start[], _data_end[];
extern char _bss_start[], _bss_end[];

extern char _kernel_reg_ctx_start[], _kernel_reg_ctx_end[];
extern char _kernel_stack_start[], _kernel_stack_end[];

extern char _heap_start[], _heap_end[],_heap_size[];



#endif