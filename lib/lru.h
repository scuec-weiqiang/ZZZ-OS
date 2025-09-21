/**
 * @FilePath: /vboot/lib/lru.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:54:03
 * @LastEditTime: 2025-09-17 23:40:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef LRU_H
#define LRU_H

#include "hashtable.h"
#include "list.h"
#include "types.h"

// LRU节点结构：嵌入哈希表节点和双向链表节点
struct lru_node
{
    struct hlist_node hnode; // 哈希表节点（用于快速查找）
    struct list lnode;       // 双向链表节点（用于维护访问顺序）
    s64 ref_count;           // 引用计数
};

typedef int (*lru_free_func_t)(struct lru_node *node);

struct lru_cache
{
    size_t capacity;      // 容量,缓存个数不能超过该值
    size_t node_count;    // 当前缓存数量
    struct hashtable *ht; // 哈希表，用于快速查找
    struct list lhead;    // 双向链表，用于维护访问顺序
    lru_free_func_t free; // 释放节点的回调函数
};

typedef int (*lru_walk_func_t)(struct lru_cache *cache, struct lru_node *node);

extern struct lru_cache *lru_init(size_t capacity, lru_free_func_t free_func, hash_func_t hash_func, hash_compare_t hash_compare);
extern void lru_node_init(struct lru_node *node);
extern void lru_destroy(struct lru_cache *cache);

extern struct lru_node *lru_hash_lookup(struct lru_cache *cache, struct lru_node *node);
extern int lru_hash_remove(struct lru_cache *cache, struct lru_node *node);
extern int lru_hash_insert(struct lru_cache *cache, struct lru_node *node);
extern int lru_cache_insert(struct lru_cache *cache, struct lru_node *node);
extern int lru_cache_remove(struct lru_cache *cache, struct lru_node *node);
extern int lru_ref(struct lru_cache *cache, struct lru_node *node);
extern int lru_unref(struct lru_cache *cache, struct lru_node *node);
extern int lru_walk(struct lru_cache *cache, lru_walk_func_t func);

#endif