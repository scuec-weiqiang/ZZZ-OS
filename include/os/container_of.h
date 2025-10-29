/**
 * @FilePath: /ZZZ/lib/os/container_of.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 22:57:35
 * @LastEditTime: 2025-08-22 21:24:07
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H


/**
 * @brief: 返回结构体成员的偏移地址
 * @param struct_type: 结构体的类型
 * @param member_name:结构体的成员名
*/
#define offsetof(struct_type,member_name)  \
    ((size_t)(&(((struct_type*)0)->member_name))) \
            
/**
 * @brief: 返回结构体成员所在的结构体的地址
 * @param member_ptr: 成员的地址
 * @param struct_type: 结构体的类型
 * @param member_name:结构体的成员名
**/   
#define container_of(member_ptr,struct_type,member_name) \
    ((struct_type*)(((size_t)member_ptr)-offsetof(struct_type,member_name))) \


#endif // CONTAINER_OF_H