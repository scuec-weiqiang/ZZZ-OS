/**
 * @FilePath: /ZZZ/lib/hashtable.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 17:00:57
 * @LastEditTime: 2025-08-28 01:41:30
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

/*
    1.  用户需要实现计算哈希值的函数，以及比较两个节点是否相等的函数。在函数内部的操作需要借助container_of宏
        hash_func_t: 计算哈希值的函数,输入参数为节点指针，返回值为哈希值
        hash_compare_t: 比较两个节点是否相等的函数，输入参数为两个节点的指针，返回值为0表示相等，非0表示不相等。

    2.  将hlist_node_t嵌入到用户自定义的结构体中，调用哈希表相关函数时只需要把该结构体内的hlist_node_t作为参数传入即可。

    3.  模块不负责释放用户自定义结构体所占的内存，用户需要自行管理。
*/

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "hlist.h"
#include "types.h"

typedef struct hashtable hashtable_t;
typedef uint64_t hval_t;

typedef hval_t (*hash_func_t)(const hlist_node_t*);
typedef int64_t (*hash_compare_t)(const hlist_node_t*, const hlist_node_t*);

hashtable_t*    hashtable_init(size_t num_buckets, hash_func_t hash_func,hash_compare_t hash_compare);
void            hashtable_destroy(hashtable_t *ht);
hlist_node_t*   hashtable_lookup(hashtable_t *ht, hlist_node_t *node);
int64_t         hashtable_insert(hashtable_t *ht, hlist_node_t *node);
int64_t         hashtable_remove(hashtable_t *ht, hlist_node_t *node);  
size_t          hashtable_size(hashtable_t *ht);
size_t          hashtable_node_count(hashtable_t *ht);

#endif // HASHTABLE_H