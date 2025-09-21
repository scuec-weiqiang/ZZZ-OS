/**
 * @FilePath: /vboot/lib/hashtable.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 17:00:57
 * @LastEditTime: 2025-09-17 23:32:41
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

typedef u64 hval_t;

typedef hval_t (*hash_func_t)(const struct hlist_node *);
typedef int (*hash_compare_t)(const struct hlist_node *, const struct hlist_node *);

struct hashtable *hashtable_init(size_t num_buckets, hash_func_t hash_func, hash_compare_t hash_compare);
void hashtable_destroy(struct hashtable *ht);
struct hlist_node *hashtable_lookup(struct hashtable *ht, struct hlist_node *node);
int hashtable_insert(struct hashtable *ht, struct hlist_node *node);
int hashtable_remove(struct hashtable *ht, struct hlist_node *node);
size_t hashtable_size(struct hashtable *ht);
size_t hashtable_node_count(struct hashtable *ht);

#endif // HASHTABLE_H