/**
 * @FilePath: /ZZZ/lib/lru.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:53:56
 * @LastEditTime: 2025-09-03 22:23:53
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "hashtable.h"
#include "list.h"
#include "check.h"
#include "utils.h"
#include "page_alloc.h"
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
lru_cache_t* lru_cache_init(size_t capacity, lru_free_func_t free_func,hash_func_t hash_func, hash_compare_t hash_compare)
{
    CHECK(capacity > 0, "Capacity must be greater than 0", return NULL;);
    // CHECK(free_func != NULL, "Free function must not be NULL", return NULL;);
    CHECK(hash_func != NULL, "Hash function must not be NULL", return NULL;);
    CHECK(hash_compare != NULL, "Key compare function must not be NULL", return NULL;);

    lru_cache_t *cache = (lru_cache_t *)malloc(sizeof(lru_cache_t));
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

/**
* @brief 销毁LRU缓存
*
* 销毁传入的LRU缓存对象，释放所有相关资源。
*
* @param cache 指向LRU缓存对象的指针
*/
void lru_cache_destroy(lru_cache_t *cache)
{
    CHECK(cache != NULL, "Cache is NULL", return;);

    list_t *pos,*n;
    list_for_each_safe(pos,n,&cache->lhead) 
    {
        lru_node_t *node = container_of(pos, lru_node_t, lnode);
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
lru_node_t* lru_cache_get(lru_cache_t *cache, lru_node_t *node)
{
    CHECK(cache != NULL, "ptr <lru_cache_t *cache> is NULL", return NULL;);
    CHECK(node != NULL, "ptr <lru_node_t *node> is NULL", return NULL;);

    hlist_node_t *hlist_node = (hlist_node_t *)hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hlist_node != NULL, "", return NULL;);
    list_mov(&cache->lhead,&container_of(hlist_node,lru_node_t,hnode)->lnode); // 将节点移动到双向链表头部，表示最近访问
    return (lru_node_t *)container_of(hlist_node, lru_node_t, hnode);
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
int64_t lru_cache_insert(lru_cache_t *cache, lru_node_t *node)
{
    CHECK(cache != NULL, "ptr <lru_cache_t *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <lru_node_t *node> is NULL", return -1;);

    if(cache->node_count >= cache->capacity) 
    {
        hashtable_remove(cache->ht, &container_of(cache->lhead.prev,lru_node_t,lnode)->hnode); // 从哈希表中移除该节点
        cache->free(container_of(cache->lhead.prev,lru_node_t,lnode));
        list_del(cache->lhead.prev); // 删除双向链表尾部的节点，即最久未访问的节点
        cache->node_count--;
    }

    hlist_node_t *hlist_node = (hlist_node_t *)hashtable_lookup(cache->ht, &node->hnode);
    if(hlist_node != NULL) // 如果该缓存已经存在
    {
        lru_node_t *old_node = container_of(hlist_node, lru_node_t, hnode);
        hashtable_remove(cache->ht,hlist_node); // 从哈希表中移除旧缓存节点
        list_del(&old_node->lnode); // 删除旧缓存
        cache->free(old_node);
    }

    int64_t ret = hashtable_insert(cache->ht, &node->hnode);
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
int64_t lru_cache_delete(lru_cache_t *cache, lru_node_t *node)
{
    CHECK(cache != NULL, "ptr <lru_cache_t *cache> is NULL", return -1;);
    CHECK(node != NULL, "ptr <lru_node_t *node> is NULL", return -1;);

    hlist_node_t *hlist_node = (hlist_node_t *)hashtable_lookup(cache->ht, &node->hnode);
    CHECK(hlist_node != NULL, "Node not found in cache", return -1;);

    hashtable_remove(cache->ht,hlist_node); // 从哈希表中移除该节点
    list_del(&container_of(hlist_node,lru_node_t,hnode)->lnode); // 从双向链表中删除该节点
    cache->free(container_of(hlist_node,lru_node_t,hnode)); // 释放节点
    cache->node_count--;

    return 0;
}


// /**************************************   test   *********************************** */
// // 测试用的自定义数据结构
// // 测试用的自定义数据结构
// typedef struct test_node {
//     uint32_t key;
//     uint32_t value;
//     lru_node_t lru_node;  // 嵌入LRU节点
// } test_node_t;
// // 哈希函数：基于age计算哈希值
// static hval_t test_hash_func(const hlist_node_t *node) 
// {
//     const test_node_t *t_node = container_of(node, test_node_t, lru_node.hnode);
//     return (hval_t)t_node->key;  // 简单使用key作为哈希值
// }

// // 比较函数：比较两个节点的key是否相等
// static int64_t test_hash_compare(const hlist_node_t *a, const hlist_node_t *b) 
// {
//     const test_node_t *node_a = container_of(a, test_node_t, lru_node.hnode);
//     const test_node_t *node_b = container_of(b, test_node_t, lru_node.hnode);
//     return (node_a->key == node_b->key) ? 0 : 1;
// }

// // 节点释放函数
// static int64_t test_free_func(lru_node_t *node) {
//     test_node_t *t_node = container_of(node, test_node_t, lru_node);
//     free(t_node);
//     return 0;
// }

// static test_node_t *create_test_node(int key, int value)
// {
//     test_node_t *node = (test_node_t *)malloc(sizeof(test_node_t));
//     if (!node) return NULL;
//     node->key = key;
//     node->value = value;
//     // hlist_node_init(&node->lru_node.hnode);  // 初始化哈希节点
//     // list_node_init(&node->lru_node.lnode);   // 初始化链表节点
//     return node;
// }

// void lru_test()
// {

//     // 1. 初始化容量为3的LRU缓存
//     lru_cache_t *cache = lru_cache_init(3, test_free_func, test_hash_func, test_hash_compare);
//     CHECK(cache != NULL,"",);
//     CHECK(lru_cache_capacity(cache) == 3,"",);
//     CHECK(lru_cache_node_count(cache) == 0,"",);
//     printf("初始化测试通过\n");

//     // 2. 插入3个节点（未达容量）
//     test_node_t *n1 = create_test_node(1, 100);
//     test_node_t *n2 = create_test_node(2, 200);
//     test_node_t *n3 = create_test_node(3, 300);

//     CHECK(lru_cache_put(cache, &n1->lru_node) == 0,"",);
//     CHECK(lru_cache_put(cache, &n2->lru_node) == 0,"",);
//     CHECK(lru_cache_put(cache, &n3->lru_node) == 0,"",);
//     CHECK(lru_cache_node_count(cache) == 3,"",);
//     printf("插入3个节点测试通过\n");

//     // 3. 查找节点并验证LRU顺序更新
//     test_node_t *find_n2 = (test_node_t *)container_of(lru_cache_get(cache, &n2->lru_node),test_node_t,lru_node);
//     CHECK(find_n2 != NULL,"",);
//     CHECK(find_n2->key == 2,"",); // 验证找到正确节点
//     printf("查找节点测试通过\n");

//     // 4. 插入第4个节点（超过容量，触发淘汰）
//     test_node_t *n4 = create_test_node(4, 400);
//     CHECK(lru_cache_put(cache, &n4->lru_node) == 0,"",);
//     CHECK(lru_cache_node_count(cache) == 3,"",); // 容量保持不变

//     // 验证最久未使用的n1被淘汰
//     test_node_t *temp = create_test_node(1, 0);            // 用于查找的临时节点
//     CHECK(lru_cache_get(cache, &temp->lru_node) == NULL,"",); // n1应已被淘汰
//     free(temp);
//     temp = NULL;
//     printf("淘汰策略测试通过\n");

//     // 5. 验证剩余节点存在性
//     temp = create_test_node(2, 0);
//     CHECK(lru_cache_get(cache, &temp->lru_node) != NULL,"",); // n2被访问过，应存在
//     free(temp);
//     temp = NULL;

//     temp = create_test_node(3, 0);
//     CHECK(lru_cache_get(cache, &temp->lru_node) != NULL,"",); // n3存在
//     free(temp);
//     temp = NULL;

//     temp = create_test_node(4, 0);
//     CHECK(lru_cache_get(cache, &temp->lru_node) != NULL,"",); // n4存在
//     free(temp);
//     temp = NULL;
//     printf("剩余节点验证测试通过\n");

//     // 6. 测试重复插入相同key（更新操作）
//     test_node_t *n2_new = create_test_node(2, 201); // 相同key，不同value
//     CHECK(lru_cache_put(cache, &n2_new->lru_node) == 0,"",);
//     CHECK(lru_cache_node_count(cache) == 3,"",); // 数量不变

//     // 验证更新后的值
//     temp = create_test_node(2, 0);
//     test_node_t *updated_node = (test_node_t *)container_of(lru_cache_get(cache, &temp->lru_node),test_node_t,lru_node);
//     CHECK(updated_node->value == 201,"",); // 验证值已更新
//     free(temp);
//     temp = NULL;
//     printf("重复插入更新测试通过\n");

//     // 7. 销毁缓存
//     lru_cache_destroy(cache);
//     printf("所有LRU测试通过!\n");
// }