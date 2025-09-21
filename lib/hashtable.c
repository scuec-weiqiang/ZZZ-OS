/**
 * @FilePath: /vboot/lib/hashtable.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:56:47
 * @LastEditTime: 2025-09-17 23:34:47
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "hlist.h"
#include "check.h"
#include "utils.h"
#include "malloc.h"

typedef u64 hval_t;

typedef size_t (*hash_func_t)(const struct hlist_node*);
typedef hval_t (*hash_compare_t)(const struct hlist_node*, const struct hlist_node*);

// 哈希表结构
struct hashtable {
    struct hlist_head *buckets;
    size_t size; //桶的数量
    size_t node_count; // 节点总数
    hash_func_t hash_func;
    hash_compare_t hash_compare;
};

/* 哈希表初始化函数 */
struct hashtable* hashtable_init(size_t num_buckets, hash_func_t hash_func, hash_compare_t hash_compare)
{
    CHECK(num_buckets > 0, "Number of buckets must be greater than 0", return NULL;);
    CHECK( is_power_of_two(num_buckets), "Number of buckets must be power of 2", return NULL;);
    CHECK(hash_func != NULL, "Hash function must not be NULL", return NULL;);
    CHECK(hash_compare != NULL, "Key compare function must not be NULL", return NULL;);

    struct hashtable *ht = (struct hashtable *)malloc(sizeof(*ht));
    CHECK(ht != NULL, "Memory allocation for hashtable failed", return NULL;);

    ht->buckets = (struct hlist_head *)malloc(sizeof(struct hlist_head) * num_buckets);
    CHECK(ht->buckets != NULL, "Memory allocation for buckets failed", return NULL;);
    for (size_t i = 0; i < num_buckets; i++) {
        hlist_head_init(&ht->buckets[i]);
    }

    ht->hash_func = hash_func;
    ht->hash_compare = hash_compare;
    ht->size = num_buckets;

    return ht;
}

/* 销毁哈希表（不销毁节点本身） */
void hashtable_destroy(struct hashtable *ht)
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return;);
    free(ht->buckets);
    ht->buckets = NULL;
    free(ht);
    ht = NULL;
}

/* 从哈希表中查找节点 */
struct hlist_node *hashtable_lookup(struct hashtable *ht, struct hlist_node *node )
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return NULL;);
    CHECK(node != NULL, "Key must not be NULL", return NULL;);

    // 计算哈希值，并找到对应的桶
    hval_t hval = ht->hash_func(node);
    struct hlist_head *bucket = &ht->buckets[hval & (ht->size-1)];

    struct hlist_node *pos = NULL;
    // 检查是否已经存在相同key的节点
    hlist_for_each(pos,bucket)
    {
        if (ht->hash_compare(node,pos) == 0)
        {
            return pos; // 返回找到的节点
        }
    }
    return NULL; // 没有找到相同key的节点，返回NULL
}

/* 插入节点到哈希表 */
int hashtable_insert(struct hashtable *ht,struct hlist_node *node)
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return -1;);
    CHECK(node != NULL, "Key must not be NULL", return -1;);

    // 计算哈希值，并找到对应的桶
    hval_t hval = ht->hash_func(node);
    struct hlist_head *bucket = &ht->buckets[hval & (ht->size-1)];

    hlist_add_head(bucket,node);
    return 0;
}

/* 从哈希表中删除节点 */
int hashtable_remove(struct hashtable *ht, struct hlist_node *node)
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return -1;);
    CHECK(node != NULL, "Key must not be NULL", return -1;);

    hlist_del(node); 

    return 0; 
}

/* 获取哈希表当前大小 */
size_t hashtable_size(struct hashtable *ht)
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return 0;);
    return ht->size;
}

/* 获取哈希表当前node数*/
size_t hashtable_node_count(struct hashtable *ht)
{
    CHECK(ht != NULL, "Hashtable must not be NULL", return 0;);
    return ht->node_count;
}
