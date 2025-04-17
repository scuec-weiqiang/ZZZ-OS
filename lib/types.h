#ifndef TYPES_H
#define TYPES_H

    #define SYSTEM_BITS 64

    #if 64==SYSTEM_BITS
        typedef char                int8_t;
        typedef unsigned char       uint8_t;
        typedef short               int16_t;
        typedef unsigned short      uint16_t;       
        typedef int                 int32_t;
        typedef unsigned int        uint32_t;
        typedef long long           int64_t;
        typedef unsigned long long  uint64_t;
        typedef uint64_t            max_uint_t;
        typedef uint64_t            addr_t;
        typedef uint64_t            size_t;
        typedef uint64_t            reg_t;
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
    
    #if 16==SYSTEM_BITS
        typedef char        int8_t;
        typedef unsigned    char uint8_t;
        typedef short       int16_t;
        typedef unsigned    short uint16_t;   
        typedef long        uint32_t;
        typedef uint32_t    max_uint_t;
        typedef uint16_t    addr_t;
        typedef uint16_t    size_t;
        typedef uint16_t    reg_t;
    #endif  

    #define NULL_PTR ((void *)0)
    #define IS_NULL_PTR(ptr)    (NULL_PTR==ptr?1:0)

    #define __SELF          static  
    #define __INLINE        inline  
    #define STATIC_INLINE   static inline 

    typedef enum
    {
        SUCCESS,// 成功
        NOT_FOUND_ERROR,// 未找到匹配的数据
        FULL_SIZE_ERROR,// 表/缓冲区已满
        INDEX_OUT_OF_BOUNDS_ERROR,// 标号索引越界
        MEMORY_ALLOCATION_ERROR,// 内存分配错误
        MEMORY_FREE_ERROR,// 内存释放错误
        NULL_POINTER_ERROR,// 空指针
        TIMEOUT_ERROR,// 超时
    }status_t;

    
#endif 