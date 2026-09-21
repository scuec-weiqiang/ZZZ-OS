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

#define ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            panic("ASSERT failed: %s, function %s, file %s, line %d: %s\n", \
                  #expr, __func__, __FILE__, __LINE__, (msg)); \
            __builtin_unreachable(); \
        } \
    } while (0)

#ifndef NDEBUG
    
    #define WARN_ONCE(expr,msg,...)\
        do{\
            static int warned = 0;\
            if(!(expr) && !warned)\
            {\
                __check_fail(#expr, __FILE__, __LINE__, __func__);\
                if(msg) { printk("%s\n", msg, ##__VA_ARGS__); }\
                warned = 1;\
            }\
        }while(0)

        #define WARN(expr,msg,...)\
        do{\
            if(!(expr))\
            {\
                __check_fail(#expr, __FILE__, __LINE__, __func__);\
                if(msg) { printk("%s\n", msg, ##__VA_ARGS__); }\
            }\
        }while(0)
#endif


#endif
