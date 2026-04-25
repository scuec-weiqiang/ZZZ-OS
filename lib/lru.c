/**
 * @FilePath: /ZZZ-OS/lib/lru.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:53:56
 * @LastEditTime: 2026-04-12 00:00:00
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include <os/check.h>
#include <os/container_of.h>
#include <os/hashtable.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/lru.h>

static int lru_cache_release_node(struct lru_cache *cache, struct lru_node *node) {
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);
    CHECK(node->hnode.pprev != NULL, "ptr <struct lru_node *node> is not hashed", return -1;);
    CHECK(!list_empty(&node->lnode), "ptr <struct lru_node *node> is not linked", return -1;);

    hashtable_remove(cache->ht, &node->hnode);
    list_del(&node->lnode);
    cache->node_count--;

    if (cache->ops != NULL && cache->ops->sync != NULL) {
        cache->ops->sync(node);
    }
    if (cache->ops != NULL && cache->ops->free != NULL) {
        cache->ops->free(node);
    }

    return 0;
}

struct lru_cache *lru_cache_create(size_t bucket_hint, struct lru_ops *lru_ops, struct hash_ops *hash_ops) {
    struct lru_cache *cache = NULL;

    CHECK(bucket_hint > 0, "Bucket hint must be greater than 0", return NULL;);
    CHECK(lru_ops != NULL, "LRU ops must not be NULL", return NULL;);
    CHECK(hash_ops != NULL, "Hash ops must not be NULL", return NULL;);

    cache = (struct lru_cache *)kmalloc(sizeof(struct lru_cache));
    CHECK(cache != NULL, "Memory allocation for LRU cache failed", return NULL;);

    cache->ht = hashtable_init(bucket_hint, hash_ops);
    CHECK(cache->ht != NULL, "Initialization of hashtable failed", kfree(cache); return NULL;);

    cache->bucket_hint = bucket_hint;
    cache->node_count = 0;
    cache->ops = lru_ops;
    INIT_LIST_HEAD(&cache->lhead);

    return cache;
}

void lru_node_reset(struct lru_node *node) {
    CHECK(node != NULL, "Node is NULL", return;);
    hlist_node_init(&node->hnode);
    INIT_LIST_HEAD(&node->lnode);
}

void lru_cache_destroy(struct lru_cache *cache) {
    struct list_head *pos = NULL, *n = NULL;

    CHECK(cache != NULL, "Cache is NULL", return;);

    list_for_each_safe(pos, n, &cache->lhead)
    {
        struct lru_node *node = container_of(pos, struct lru_node, lnode);
        lru_cache_release_node(cache, node);
    }

    hashtable_destroy(cache->ht);
    kfree(cache);
}

int lru_cache_touch(struct lru_cache *cache, struct lru_node *node) {
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);
    CHECK(node->hnode.pprev != NULL, "ptr <struct lru_node *node> is not hashed", return -1;);
    CHECK(!list_empty(&node->lnode), "ptr <struct lru_node *node> is not linked", return -1;);

    list_mov(&cache->lhead, &node->lnode);

    return 0;
}

struct lru_node *lru_cache_find(struct lru_cache *cache, struct lru_node *node) {
    struct hlist_node *hnode = NULL;
    struct lru_node *found = NULL;

    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return NULL;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return NULL;);

    hnode = hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hnode != NULL, "", return NULL;);

    found = container_of(hnode, struct lru_node, hnode);
    lru_cache_touch(cache, found);
    return found;
}

int lru_cache_evict_tail(struct lru_cache *cache) {
    struct lru_node *victim = NULL;

    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    if (cache->node_count == 0 || list_empty(&cache->lhead)) {
        return 0;
    }

    victim = container_of(cache->lhead.prev, struct lru_node, lnode);
    return lru_cache_release_node(cache, victim);
}

int lru_cache_add(struct lru_cache *cache, struct lru_node *node)
{
    int ret = 0;

    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);
    CHECK(node->hnode.pprev == NULL, "ptr <struct lru_node *node> already hashed", return -1;);
    CHECK(list_empty(&node->lnode), "ptr <struct lru_node *node> already linked", return -1;);

    ret = hashtable_insert(cache->ht, &node->hnode);
    if (ret != 0) {
        return ret;
    }

    list_add(&cache->lhead, &node->lnode);
    cache->node_count++;

    return 0;
}

int lru_cache_remove(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);

    return lru_cache_release_node(cache, node);
}

int lru_cache_walk(struct lru_cache *cache, lru_walk_func_t func)
{
    struct lru_node *pos = NULL, *n = NULL;

    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(func != NULL, "ptr <lru_walk_func_t func> is NULL", return -1;);

    list_for_each_entry_safe(pos, n, &cache->lhead, struct lru_node, lnode)
    {
        func(pos);
    }
    return 0;
}
