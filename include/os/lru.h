/**
 * @FilePath: /ZZZ-OS/lib/os/lru.h
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:54:03
 * @LastEditTime: 2025-10-06 18:58:43
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#ifndef LRU_H
#define LRU_H

#include <os/hashtable.h>
#include <os/list.h>
#include <os/types.h>

// LRU 节点结构：嵌入哈希表节点和双向链表节点
struct lru_node {
    struct hlist_node hnode; // 哈希表节点（用于快速查找）
    struct list_head lnode; // 双向链表节点（用于维护活跃访问顺序）
};

struct lru_ops {
    int (*free)(struct lru_node *node);
    int (*sync)(struct lru_node *node);
};

struct lru_cache {
    size_t bucket_hint;   // 哈希桶数量建议值，不限制缓存总数
    size_t node_count;    // 当前缓存数量
    struct hashtable *ht; // 哈希表，用于快速查找
    struct list_head lhead; // 双向链表，用于维护活跃访问顺序
    struct lru_ops *ops;
};

typedef int (*lru_walk_func_t)(struct lru_node *node);

extern struct lru_cache *lru_cache_create(size_t bucket_hint, struct lru_ops *lru_ops, struct hash_ops *hash_ops);

extern void lru_node_reset(struct lru_node *node);
extern void lru_cache_destroy(struct lru_cache *cache);

extern int lru_cache_touch(struct lru_cache *cache, struct lru_node *node);
extern struct lru_node *lru_cache_find(struct lru_cache *cache, struct lru_node *key);
extern int lru_cache_add(struct lru_cache *cache, struct lru_node *node);
extern int lru_cache_remove(struct lru_cache *cache, struct lru_node *node);
extern int lru_cache_evict_tail(struct lru_cache *cache);
extern int lru_cache_walk(struct lru_cache *cache, lru_walk_func_t func);

#endif
