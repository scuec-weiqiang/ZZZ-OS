/**
 * @FilePath: /ZZZ-OS/lib/os/hashtable.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 17:00:57
 * @LastEditTime: 2025-10-06 16:36:15
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

#include <os/hlist.h>
#include <os/types.h>

typedef uint64_t hval_t;

struct hash_ops
{
    hval_t (*hash_func)(const struct hlist_node *);
    int (*hash_compare)(const struct hlist_node *, const struct hlist_node *);
};

// 哈希表结构
struct hashtable {
    struct hlist_head *buckets;
    size_t size; //桶的数量
    size_t node_count; // 节点总数
    struct hash_ops *ops; // 哈希函数和比较函数
};

struct hashtable *hashtable_init(size_t num_buckets, struct hash_ops *ops);
void hashtable_destroy(struct hashtable *ht);
struct hlist_node *hashtable_lookup(struct hashtable *ht, struct hlist_node *node);
int hashtable_insert(struct hashtable *ht, struct hlist_node *node);
int hashtable_remove(struct hashtable *ht, struct hlist_node *node);
size_t hashtable_size(struct hashtable *ht);
size_t hashtable_node_count(struct hashtable *ht);

#endif // HASHTABLE_H