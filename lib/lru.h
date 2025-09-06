/**
 * @FilePath: /ZZZ/lib/lru.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:54:03
 * @LastEditTime: 2025-09-03 22:23:14
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef LRU_H
#define LRU_H

#include "types.h"
#include "list.h"
#include "hashtable.h"

// LRU节点结构：嵌入哈希表节点和双向链表节点
typedef struct lru_node {
    hlist_node_t hnode;      // 哈希表节点（用于快速查找）
    list_t lnode;       // 双向链表节点（用于维护访问顺序）
} lru_node_t;

typedef int64_t (*lru_free_func_t)(lru_node_t *node);

typedef struct lru_cache
{
    size_t capacity;       // 容量,缓存个数不能超过该值
    size_t node_count;       // 当前缓存数量
    hashtable_t *ht;        // 哈希表，用于快速查找
    list_t lhead;          // 双向链表，用于维护访问顺序
    lru_free_func_t free;      // 释放节点的回调函数
}lru_cache_t;

extern lru_cache_t* lru_cache_init(size_t capacity, lru_free_func_t free, hash_func_t hash_func, hash_compare_t hash_compare);
extern void lru_cache_destroy(lru_cache_t *cache);
extern int64_t lru_cache_insert(lru_cache_t *cache, lru_node_t *node);
extern lru_node_t* lru_cache_get(lru_cache_t *cache, lru_node_t *node);
extern int64_t lru_cache_delete(lru_cache_t *cache, lru_node_t *node);

#endif // LRU_H