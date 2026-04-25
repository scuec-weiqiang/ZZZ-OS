/**
 * @FilePath: /ZZZ-OS/lib/hashtable.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 14:56:47
 * @LastEditTime: 2025-10-06 16:36:02
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <os/hlist.h>
#include <os/check.h>
#include <os/utils.h>
#include <os/kmalloc.h>
#include <os/hashtable.h>
#include <os/err.h>

/* 哈希表初始化函数 */
struct hashtable* hashtable_init(size_t num_buckets, struct hash_ops *ops) {
    CHECK(num_buckets > 0, "Number of buckets must be greater than 0", return NULL;);
    CHECK(ops != NULL, "Hash operations must not be NULL", return NULL;);
    CHECK(ops->hash_func != NULL, "Hash function must not be NULL", return NULL;);
    CHECK(ops->hash_compare != NULL, "Key compare function must not be NULL", return NULL;);
    num_buckets = next_power_of_two(num_buckets); // 确保桶数量是2的幂次方

    struct hashtable *ht = (struct hashtable *)kmalloc(sizeof(*ht));
    CHECK(ht != NULL, "Memory allocation for hashtable failed", return NULL;);

    ht->buckets = (struct hlist_head *)kmalloc(sizeof(struct hlist_head) * num_buckets);
    CHECK(ht->buckets != NULL, "Memory allocation for buckets failed", return NULL;);
    for (size_t i = 0; i < num_buckets; i++) {
        hlist_head_init(&ht->buckets[i]);
    }

    ht->ops = ops; // 复制哈希函数和比较函数
    ht->node_count = 0;

    ht->size = num_buckets;

    return ht;
}

/* 销毁哈希表（不销毁节点本身） */
void hashtable_destroy(struct hashtable *ht) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return;);
    kfree(ht->buckets);
    ht->buckets = NULL;
    kfree(ht);
    ht = NULL;
}

/* 从哈希表中查找节点 */
struct hlist_node *hashtable_lookup(struct hashtable *ht, struct hlist_node *node ) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return NULL;);
    CHECK(node != NULL, "Key must not be NULL", return NULL;);

    // 计算哈希值，并找到对应的桶
    hval_t hval = ht->ops->hash_func(node);
    struct hlist_head *bucket = &ht->buckets[hval & (ht->size-1)];

    struct hlist_node *pos = NULL;
    // 检查是否已经存在相同key的节点
    hlist_for_each(pos,bucket)
    {
        if (ht->ops->hash_compare(node,pos) == 0)
        {
            return pos; // 返回找到的节点
        }
    }
    return NULL; // 没有找到相同key的节点，返回NULL
}

/* 插入节点到哈希表 */
int hashtable_insert(struct hashtable *ht,struct hlist_node *node) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return -EINVAL;);
    CHECK(node != NULL, "Key must not be NULL", return -EINVAL;);

    // 计算哈希值，并找到对应的桶
    hval_t hval = ht->ops->hash_func(node);
    struct hlist_head *bucket = &ht->buckets[hval & (ht->size-1)];

    hlist_add_head(bucket,node);
    ht->node_count++;
    return 0;
}

/* 从哈希表中删除节点 */
int hashtable_remove(struct hashtable *ht, struct hlist_node *node) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return -1;);
    CHECK(node != NULL, "Key must not be NULL", return -1;);

    hlist_del(node); 
    ht->node_count--;
    
    return 0; 
}

/* 获取哈希表当前大小 */
size_t hashtable_size(struct hashtable *ht) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return 0;);
    return ht->size;
}

/* 获取哈希表当前node数*/
size_t hashtable_node_count(struct hashtable *ht) {
    CHECK(ht != NULL, "Hashtable must not be NULL", return 0;);
    return ht->node_count;
}
