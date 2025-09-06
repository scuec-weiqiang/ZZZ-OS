/**
 * @FilePath: /ZZZ/lib/types.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-04-15 17:27:48
 * @LastEditTime: 2025-08-26 16:10:26
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _TYPES_H
#define _TYPES_H

    #define SYSTEM_BITS 64
    #define GNUC

    #if 64==SYSTEM_BITS
        typedef char                int8_t;
        typedef unsigned char       uint8_t;
        typedef short               int16_t;
        typedef unsigned short      uint16_t;       
        typedef int                 int32_t;
        typedef unsigned int        uint32_t;
        typedef long                int64_t;
        typedef unsigned long       uint64_t;
        typedef uint64_t            max_uint_t;
        typedef uint64_t            addr_t;
        typedef uint64_t            size_t;
        typedef int64_t             ssize_t;
        typedef uint64_t            reg_t;
        typedef uint32_t            dev_t;
        typedef long long           loff_t;
        typedef unsigned long       uintptr_t;
        #define UINT8_MAX     (0xff)
        #define UINT16_MAX    (0xffff)
        #define UINT32_MAX    (0xffffffff)
        #define UINT64_MAX    (0xffffffffffffffff)

    #endif 

    #if 32==SYSTEM_BITS
        typedef char                 int8_t;
        typedef unsigned char        uint8_t;
        typedef short                int16_t;
        typedef unsigned short       uint16_t;       
        typedef int                  int32_t;
        typedef unsigned int         uint32_t;
        typedef long long            int64_t;
        typedef unsigned long long   uint64_t;
        typedef uint64_t             max_uint_t;
        typedef uint32_t             addr_t;
        typedef uint32_t             size_t;
        typedef uint32_t             reg_t;
    #endif 

    typedef enum { false, true } bool;

    #define NULL ((void *)0)
    #define IS_NULL_PTR(ptr)    (NULL==ptr?1:0)

    #define __SELF          static  
    #define __INLINE        inline  
    #define STATIC_INLINE   static inline 
    #define __PROTECT(x)       do{x}while(0)

#ifdef GNUC
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif 




#endif