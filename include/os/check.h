/**
 * @FilePath: /ZZZ/lib/os/check.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-06-04 16:51:09
 * @LastEditTime: 2025-08-26 19:30:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef _CHECK_H
#define _CHECK_H

 // 使用你的打印函数
#include <os/printk.h> 

static inline void __check_fail(const char *expr, const char *file, int line, const char *func)
{
    printk("Check failed: %s, function %s, file %s, line %d\n", expr, func, file, line);
}

// #define NDEBUG

#ifdef NDEBUG
    #define CHECK(expr,msg,ret)\
        do{\
            if(!(expr))\
            {\
                if(msg) { printk("%s\n", msg); }\
                ret\
            }\
        }while(0)
#else
    #define CHECK(expr,msg,ret)\
        do{\
            if(!(expr))\
            {\
                __check_fail(#expr, __FILE__, __LINE__, __func__);\
                if(msg) { printk("%s\n", msg); }\
                ret\
            }\
        }while(0)
#endif


#endif