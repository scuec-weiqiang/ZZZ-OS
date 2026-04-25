/**
 * @FilePath: /ZZZ-OS/include/os/types.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 19:42:44
 * @LastEditTime: 2025-12-04 21:17:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/


#ifndef _TYPES_H
#define _TYPES_H

#include <os/config.h>

#if SYS_BITS == 64
    typedef signed char         int8_t;    
    typedef signed short        int16_t;       
    typedef signed int          int32_t;
    typedef signed long         int64_t;
    typedef signed long         ssize_t;
    typedef int64_t             intptr_t;

    typedef unsigned char       uint8_t;    
    typedef unsigned short      uint16_t;       
    typedef unsigned int        uint32_t;
    typedef unsigned long       uint64_t;
    typedef unsigned long       size_t;
    typedef uint64_t            uintptr_t;
    #define UINT_MAX            (0xffffffffffffffff)

#elif SYS_BITS == 32
    typedef signed char         int8_t;    
    typedef signed short        int16_t;       
    typedef signed int          int32_t;
    typedef signed long long    int64_t;
    typedef signed int          ssize_t;
    typedef int32_t             intptr_t;

    typedef unsigned char       uint8_t;    
    typedef unsigned short      uint16_t;       
    typedef unsigned int        uint32_t;
    typedef unsigned long long  uint64_t;
    typedef unsigned int        size_t;
    typedef uint32_t            uintptr_t;
    #define UINT_MAX            (0xffffffff)
    #define INT_MAX             (0x7fffffff)
#endif

#define UINT8_MAX     (0xff)
#define UINT16_MAX    (0xffff)
#define UINT32_MAX    (0xffffffff)
#define UINT64_MAX    (0xffffffffffffffff)

#define INT8_MAX      (0x7f)
#define INT16_MAX     (0x7fff)
#define INT32_MAX     (0x7fffffff)
#define INT64_MAX     (0x7fffffffffffffff)

typedef uintptr_t     reg_t;
typedef uintptr_t     phys_addr_t;
typedef uintptr_t     virt_addr_t;
typedef uintptr_t     addr_t;

typedef enum { false, true } bool;

#define NULL ((void *)0)

#define __PROTECT(x)       do{x}while(0)

#ifdef GNUC
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif 

#define asm __asm__

#define __in
#define __out
#define __user

typedef uint32_t dev_t;

typedef uint8_t  __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;

typedef unsigned char __u8;
typedef uint16_t  __le16;
typedef uint16_t  __be16;
typedef uint32_t  __le32;
typedef uint32_t  __be32;
typedef uint64_t  __le64;
typedef uint64_t  __be64;


typedef struct {
	int counter;
} atomic_t;

#if SYS_BITS == 64
typedef struct {
	long counter;
} atomic64_t;
#endif

struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};


#endif