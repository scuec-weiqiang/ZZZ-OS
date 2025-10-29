/**
 * @FilePath: /vboot/lib/os/hlist.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-17 23:13:18
 * @LastEditTime: 2025-09-17 23:30:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/

#ifndef HASH_H
#define HASH_H

#include "os/types.h"

struct hlist_node
{
    struct hlist_node *next;
    struct hlist_node **pprev;// 指向前一个节点的 next 指针的地址
};

struct hlist_head
{
    struct hlist_node *first;
};


static inline void hlist_node_init(struct hlist_node *h) 
{
    h->next = (struct hlist_node*)NULL;
    h->pprev = (struct hlist_node**)NULL;
}

static inline void hlist_head_init(struct hlist_head *h) 
{
    h->first = (struct hlist_node*)NULL;
}

static inline int hlist_head_empty(struct hlist_head *h) 
{
    return !h->first;
}

static inline void hlist_add_head(struct hlist_head *h, struct hlist_node *n) 
{
    // 两种情况,一种是头结点后面有节点，一种是空
    // head->n1->... 执行完后变为 head->n->n1->... 这种情况下要设置n1d的pprev指向n->next，即n1->next
    // 但是有可能头结点后是空的head->NULL，则变为head->n->NULL,这种情况下要将n的pprev指向head->first，即NULL
    struct hlist_node *n1 = h->first;

    // 设置新节点指针
    n->next = n1;
    n->pprev = &h->first;

    // 如果头结点后面是有节点的，则将n1的pprev指针指向新节点的next，即n->next
    if(n1) 
    {
        n1->pprev = &n->next; 
    }

    h->first = n;
}

static inline void hlist_del(struct hlist_node *n) 
{
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;

    // 如果有下一个节点，则将下一个节点的pprev指针指向当前节点的pprev
    if(next) 
    {
        next->pprev = pprev; 
    }

    // 将前一个节点的next指针指向当前节点的next
    *pprev = next;

    // 清空当前节点的指针
    n->next = (struct hlist_node*)NULL;
    n->pprev = (struct hlist_node**)NULL;
}

// 遍历哈希桶
#define hlist_for_each(pos, head) \
    for (pos = (head)->first; pos; pos = pos->next)

// 遍历哈希桶（安全版本，允许在遍历时删除节点）
#ifdef __GNUC__
#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first; pos && ({ n = pos->next; 1; }); pos = n)
#else
#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first, n = pos ? pos->next : NULL; pos; pos = n, n = pos ? pos->next : NULL)
#endif
#endif