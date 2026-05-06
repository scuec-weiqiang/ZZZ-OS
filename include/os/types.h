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
    typedef signed char         s8;
    typedef signed short        s16;
    typedef signed int          s32;
    typedef signed long         s64;
    typedef signed char         int8_t;    
    typedef signed short        int16_t;       
    typedef signed int          s32;
    typedef signed long         s64;
    typedef signed long         ssize_t;
    typedef s64             intptr_t;

    typedef unsigned char       u8;
    typedef unsigned short      u16;
    typedef unsigned int        u32;
    typedef unsigned long       u64;
    typedef unsigned char       u8;    
    typedef unsigned short      u16;       
    typedef unsigned int        u32;
    typedef unsigned long       u64;
    typedef unsigned long       size_t;
    typedef u64            uintptr_t;
    #define INT_MAX             (0x7fffffff)
    #define UINT_MAX            (0xffffffffffffffff)

#elif SYS_BITS == 32
    typedef signed char         s8;
    typedef signed short        s16;
    typedef signed int          s32;
    typedef signed long long    s64;   
    typedef signed int          ssize_t;
    typedef s32                 intptr_t;
    #define INT_MAX             (0x7fffffff)

    typedef unsigned char       u8;
    typedef unsigned short      u16;
    typedef unsigned int        u32;
    typedef unsigned long long  u64;
    typedef unsigned int        size_t;
    typedef u32                 uintptr_t;
    #define UINT_MAX            (0xffffffff)
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
#define __iomem

typedef u32 dev_t;

#define MINORBITS	20
#define MINORMASK	((1U << MINORBITS) - 1)

#define MAJOR(dev)	((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)	((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))

typedef u8  __u8;
typedef u16 __u16;
typedef u32 __u32;
typedef u64 __u64;

typedef unsigned char __u8;
typedef u16  __le16;
typedef u16  __be16;
typedef u32  __le32;
typedef u32  __be32;
typedef u64  __le64;
typedef u64  __be64;


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
