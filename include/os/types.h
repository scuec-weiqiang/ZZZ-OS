/**
 * @FilePath: /ZZZ-OS/include/os/types.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 19:42:44
 * @LastEditTime: 2025-11-12 15:45:19
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _TYPES_H
#define _TYPES_H

    #define RISCV64
    #define GNUC

    #ifdef RISCV64
        typedef signed char         int8_t;    
        typedef signed short        int16_t;       
        typedef signed int          int32_t;
        typedef signed long         int64_t;
        typedef signed long         ssize_t;
        typedef signed long         intptr_t;

        typedef unsigned char       uint8_t;    
        typedef unsigned short      uint16_t;       
        typedef unsigned int        uint32_t;
        typedef unsigned long       uint64_t;
        typedef unsigned long       size_t;
        typedef unsigned long       uintptr_t;

    #endif 

    #define UINT8_MAX     (0xff)
    #define UINT16_MAX    (0xffff)
    #define UINT32_MAX    (0xffffffff)
    #define UINT64_MAX    (0xffffffffffffffff)

    typedef uintptr_t                 reg_t;
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

typedef uint32_t dev_t;

#endif