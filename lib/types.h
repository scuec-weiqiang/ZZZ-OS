/**
 * @FilePath: /ZZZ-OS/lib/types.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 19:42:44
 * @LastEditTime: 2025-10-04 17:08:40
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _TYPES_H
#define _TYPES_H

    #define RISCV64
    #define GNUC

    #ifdef RISCV64
        typedef signed char         s8;    
        typedef signed short        s16;       
        typedef signed int          s32;
        typedef signed long         s64;
        typedef signed long         ssize_t;
        typedef signed long         intptr_t;

        typedef unsigned char       u8;    
        typedef unsigned short      u16;       
        typedef unsigned int        u32;
        typedef unsigned long       u64;
        typedef unsigned long       size_t;
        typedef unsigned long       uintptr_t;

        #define U8_MAX     (0xff)
        #define U16_MAX    (0xffff)
        #define U32_MAX    (0xffffffff)
        #define U64_MAX    (0xffffffffffffffff)

        typedef u64                 reg_t;
    #endif 

    typedef enum { false, true } bool;

    #define NULL ((void *)0)
    
    #define __PROTECT(x)       do{x}while(0)

#ifdef GNUC
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif 

#define __in
#define __out
#define __user

#endif