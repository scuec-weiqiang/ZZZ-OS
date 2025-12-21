/**
 * @FilePath: /ZZZ-OS/include/os/container_of.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 22:57:35
 * @LastEditTime: 2025-11-15 00:36:02
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef CONTAINER_OF_H
#define CONTAINER_OF_H


/**
 * @brief: 返回结构体成员的偏移地址
 * @param struct_type: 结构体的类型
 * @param struct_member:结构体的成员名
*/
#define offsetof(struct_type,struct_member)  \
    ((size_t)(&(((struct_type*)0)->struct_member))) \
            
/**
 * @brief: 返回结构体成员所在的结构体的地址
 * @param member_ptr: 成员的地址
 * @param struct_type: 结构体的类型
 * @param struct_member:结构体的成员名
**/   
#define container_of(member_ptr,struct_type,struct_member) \
    ((struct_type*)(((size_t)(member_ptr)) - offsetof(struct_type, struct_member))) \


#endif // CONTAINER_OF_H