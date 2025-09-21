/**
 * @FilePath: /vboot/lib/lru.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:53:56
 * @LastEditTime: 2025-09-17 23:38:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "hashtable.h"
#include "list.h"
#include "check.h"
#include "utils.h"
#include "malloc.h"
#include "lru.h"

/**
* @brief 初始化LRU缓存
*
* 初始化一个LRU缓存对象。
*
* @param capacity 缓存容量
* @param free 释放缓存节点的回调函数
* @param hash_func 哈希函数
* @param hash_compare 哈希比较函数
*
* @return 返回初始化的LRU缓存对象指针，如果初始化失败则返回NULL
*/
struct lru_cache* lru_init(size_t capacity, lru_free_func_t free_func,hash_func_t hash_func, hash_compare_t hash_compare)
{
    CHECK(capacity > 0, "Capacity must be greater than 0", return NULL;);
    // CHECK(free_func != NULL, "Free function must not be NULL", return NULL;);
    CHECK(hash_func != NULL, "Hash function must not be NULL", return NULL;);
    CHECK(hash_compare != NULL, "Key compare function must not be NULL", return NULL;);

    struct lru_cache *cache = (struct lru_cache *)malloc(sizeof(struct lru_cache));
    CHECK(cache != NULL, "Memory allocation for LRU cache failed", return NULL;);

    cache->ht = hashtable_init(next_power_of_two(capacity), hash_func, hash_compare);
    CHECK(cache->ht != NULL, "Initialization of hashtable failed", free(cache);return NULL;);

    cache->capacity = capacity;
    INIT_LIST_HEAD(&cache->lhead); // 初始化双向链表头
    if(free_func != NULL)
    {
        cache->free = free_func; // 设置释放节点的回调函数
    }
    cache->node_count = 0;

    return cache;

}

void lru_node_init(struct lru_node *node)
{
    CHECK(node != NULL, "Node is NULL", return;);
    hlist_node_init(&node->hnode);
    INIT_LIST_HEAD(&node->lnode);
    node->ref_count = 0;
}
/**
* @brief 销毁LRU缓存
*
* 销毁传入的LRU缓存对象，释放所有相关资源。
*
* @param cache 指向LRU缓存对象的指针
*/
void lru_destroy(struct lru_cache *cache)
{
    CHECK(cache != NULL, "Cache is NULL", return;);

    struct list *pos,*n;
    list_for_each_safe(pos,n,&cache->lhead) 
    {
        struct lru_node *node = container_of(pos, struct lru_node, lnode);
        cache->free(node); // 释放每个节点
    }
    hashtable_destroy(cache->ht);
    free(cache);
}

/**
* @brief 在LRU缓存中查找节点
*
* 在LRU缓存中查找指定的节点，并返回该节点的指针。
*
* @param cache LRU缓存的指针
* @param node 要查找的节点的指针
*
* @return 如果找到了节点，则返回该节点的指针；否则返回NULL
*
*/
struct lru_node* lru_hash_lookup(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return NULL;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return NULL;);

    struct hlist_node *hlist_node = (struct hlist_node *)hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hlist_node != NULL, "", return NULL;);
    struct list *hlist_node_lru = &container_of(hlist_node,struct lru_node,hnode)->lnode;
    if(hlist_node_lru->prev != NULL && hlist_node_lru->next != NULL) // 如果节点在双向链表中，那么将其移动到链表头部
    {
        list_mov(&cache->lhead,hlist_node_lru); // 将节点添加到双向链表头部，表示最近访问
    }

    return (struct lru_node *)container_of(hlist_node, struct lru_node, hnode);
}


int lru_hash_insert(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);

    int ret = hashtable_insert(cache->ht, &node->hnode);
    CHECK(ret >= 0, "", return -1;);
    return 0;
}

int lru_hash_remove(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);

    struct hlist_node *hlist_node = (struct hlist_node *)hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hlist_node != NULL, "Node not found in cache", return -1;);

    hashtable_remove(cache->ht,hlist_node); // 从哈希表中移除该节点
    struct list *hlist_node_lru = &container_of(hlist_node,struct lru_node,hnode)->lnode;
    if(hlist_node_lru->prev != NULL && hlist_node_lru->next != NULL) // 如果节点在双向链表中，那么也将其从链表中删除
    {
        list_del(hlist_node_lru); // 从双向链表中删除该节点
    }
    cache->free(container_of(hlist_node,struct lru_node,hnode)); // 释放节点
    cache->node_count--;

    return 0;
}


/**
* @brief 将节点插入到LRU缓存中
*
* 将给定的节点插入到LRU缓存中。如果缓存已满，将删除最久未访问的节点。
*
* @param cache LRU缓存指针
* @param node 待插入的节点指针
*
* @return 插入成功返回0，失败返回-1
*/
int lru_cache_insert(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);

    if(cache->node_count >= cache->capacity) 
    {
        hashtable_remove(cache->ht, &container_of(cache->lhead.prev,struct lru_node,lnode)->hnode); // 从哈希表中移除最久未访问的节点
        list_del(cache->lhead.prev); // 删除最久未访问的节点
        cache->free(container_of(cache->lhead.prev,struct lru_node,lnode));
        cache->node_count--;
    }

    int ret = hashtable_insert(cache->ht, &node->hnode);
    CHECK(ret >= 0, "", return -1;);
    list_add(&cache->lhead,&node->lnode); // 将节点添加到双向链表头部，表示最近访问
    cache->node_count++;
    
    return 0;
}

/**
* @brief 从LRU缓存中删除指定的节点
*
* 从LRU缓存中删除指定的节点，并从哈希表中移除该节点，从双向链表中删除该节点，释放节点内存，并更新缓存节点计数。
*
* @param cache LRU缓存对象指针
* @param node 要删除的节点指针
*
* @return 删除成功返回0，失败返回-1
*/
int lru_cache_remove(struct lru_cache *cache, struct lru_node *node)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <struct lru_node *node> is NULL", return -1;);

    struct hlist_node *hlist_node = (struct hlist_node *)hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hlist_node != NULL, "Node not found in cache", return -1;);

    hashtable_remove(cache->ht,hlist_node); // 从哈希表中移除该节点
    list_del(&container_of(hlist_node,struct lru_node,hnode)->lnode); // 从双向链表中删除该节点
    cache->free(container_of(hlist_node,struct lru_node,hnode)); // 释放节点
    cache->node_count--;

    return 0;
}

/**
 * lru_cache_ref - 增加缓存项的引用计数
 * @cache: 缓存对象
 * @key: 要引用的键
 * 
 * 返回: 成功返回0，失败返回错误码
 */
int lru_ref(struct lru_cache *cache,struct lru_node *node)
{
    CHECK(node != NULL, "lru_cache_ref: node is NULL", return -1;);
    return node->ref_count++;
}

/**
 * lru_cache_unref - 减少缓存项的引用计数
 * @cache: 缓存对象
 * @key: 要释放引用的键
 * 
 * 当引用计数降为0时，项可能被放入LRU链表等待淘汰
 * 返回: 成功返回0，失败返回错误码
 */
int lru_unref(struct lru_cache *cache,struct lru_node *node)
{
    CHECK(node != NULL, "lru_cache_ref: node is NULL", return -1;);
    if(node->ref_count > 0)
    {
        node->ref_count--;
        if(node->ref_count == 0)
        {
            lru_cache_insert(cache, node);
        }
    }
    return 0;
}

int lru_walk(struct lru_cache *cache, lru_walk_func_t func)
{
    CHECK(cache != NULL, "ptr <struct lru_cache *cache> is NULL", return -1;);
    CHECK(func != NULL, "ptr <lru_walk_func_t func> is NULL", return -1;);

    // 遍历全局inode缓存，写回所有脏inode
    struct lru_node *pos, *n;
    list_for_each_entry_safe(pos, n, &cache->lhead, struct lru_node, lnode) 
    {
        func(cache,pos);
        // list_mov_tail(&cache->lhead, &pos->lnode);
    }

    return 0;
}