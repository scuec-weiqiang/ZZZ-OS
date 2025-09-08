/**
 * @FilePath: /ZZZ/lib/lru.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:54:03
 * @LastEditTime: 2025-09-07 20:07:15
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
    int64_t ref_count;  // 引用计数
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

typedef int64_t (*lru_walk_func_t)(lru_cache_t *cache,lru_node_t *node);

extern lru_cache_t* lru_init(size_t capacity, lru_free_func_t free, hash_func_t hash_func, hash_compare_t hash_compare);
extern void lru_node_init(lru_node_t *node);
extern void lru_destroy(lru_cache_t *cache);

extern lru_node_t*  lru_hash_lookup(lru_cache_t *cache, lru_node_t *node);
extern int64_t      lru_hash_remove(lru_cache_t *cache, lru_node_t *node);
extern int64_t      lru_hash_insert(lru_cache_t *cache, lru_node_t *node);
extern int64_t      lru_cache_insert(lru_cache_t *cache, lru_node_t *node);
extern int64_t      lru_cache_remove(lru_cache_t *cache, lru_node_t *node);
extern int64_t      lru_ref(lru_cache_t *cache,lru_node_t *node);
extern int64_t      lru_unref(lru_cache_t *cache,lru_node_t *node);
extern int64_t      lru_walk(lru_cache_t *cache, lru_walk_func_t func);

#endif