#ifndef _LIST_H
#define _LIST_H

#include "container_of.h"

typedef struct list
{
    struct list *prev;
    struct list *next;
}list_t;

#define LIST_HEAD_INIT(name)  {&(name),&(name)} 
   
// 用这个宏声明的变量会自动初始化，不需要手动调用INIT_LIST_HEAD函数来初始化链表头
#define LIST_HEAD(name) \
    list_t name = LIST_HEAD_INIT(name); \

// 这玩意可以用来声明一个不带初始化的链表头，用这个宏声明的变量需要用INIT_LIST_HEAD初始化
#define THIS_IS_LIST_HEAD(name) \
    list_t name; \

#define INIT_LIST_HEAD(ptr) do{ \
    (ptr)->prev = (ptr); \
    (ptr)->next = (ptr); \
}while(0); 

/***************************************************************
 * @description: 在两个节点之间插入新节点
 * @param {list_t} *node [in]:  需要插入的节点的指针
 * @param {list_t} *prev [in]:  插入位置的前一个节点的指针
 * @param {list_t} *next [in]:  插入位置的后一个节点的指针
 * @return {*}
***************************************************************/
STATIC_INLINE void __list_add(list_t *node,list_t *prev,list_t *next)
{   
    prev->next = node;
    node->prev = prev;
    next->prev = node;
    node->next = next;
}

/***************************************************************
 * @description: 卸载两个节点之间的节点
 * @param {list_t} *prev [in]:  卸载位置的前一个节点的指针 
 * @param {list_t} *next [in]:  卸载位置的后一个节点的指针
 * @return {*}
***************************************************************/
STATIC_INLINE void __list_del(list_t *prev,list_t *next)
{
    prev->next = next;
    next->prev = prev;
}


/***************************************************************
 * @description: 将链表<head>合并到某一链表节点<node>的后面
 * @param {list_t} *head [in]:  被合并的链表头的指针
 * @param {list_t} *node [in]:  需要合并位置的节点的指针
 * @return {*}
***************************************************************/
STATIC_INLINE void __list_splice(list_t *head,list_t *node)
{
    list_t *first = head->next;
    list_t *last =  head->prev;
    list_t *at = node->next;

    node->next = first;
    first->prev = node;
    last->next = at;
    at->prev = last;
}


/***************************************************************
 * @description: 在链表头部插入新节点
 * @param {list_t} *node [in]:  需要插入的新节点的指针
 * @param {list_t} *head [in]:  头节点
 * @return {*}
***************************************************************/
STATIC_INLINE  void list_add(list_t *head,list_t *node)
{
    __list_add(node,head,head->next);
}


/***************************************************************
 * @description: 在链表尾部插入新节点
 * @param {list_t} *node [in]:  需要插入的新节点
 * @param {list_t} *head [in]:  头节点
 * @return {*}
***************************************************************/
STATIC_INLINE void list_add_tail(list_t *head,list_t *node)
{
    __list_add(node,head->prev,head);
}


/***************************************************************
 * @description:删除节点
 * @param {list_t} *node [in]:  需要删除的节点
 * @return {*}
***************************************************************/
STATIC_INLINE void list_del(list_t *node)
{
    __list_del(node->prev,node->next);
    node->prev = NULL;
    node->next = NULL;
}


/***************************************************************
 * @description: 将一个链表节点卸下，插入到链表头部
 * @param {list_t} *node [in]:  需要卸下的节点
 * @param {list_t} *head [in]:  头节点
 * @return {*}
***************************************************************/
STATIC_INLINE void list_mov(list_t *head,list_t *node)
{
    __list_del(node->prev,node->next);
    __list_add(node,head,head->next);
}


/***************************************************************
 * @description: 将一个链表节点卸下，插入到链表尾部
 * @param {list_t} *node [in]:  需要卸下的节点
 * @param {list_t} *head [in]:  头节点
 * @return {*}
***************************************************************/
STATIC_INLINE void list_mov_tail(list_t *head,list_t *node)
{
    __list_del(node->prev,node->next);
    __list_add(node,head->prev,head);
}


/***************************************************************
 * @description: 判断链表是否为空
 * @param {list} *head [in/out]:  
 * @return {*}
***************************************************************/
STATIC_INLINE int list_empty(const list_t *head)
{
    return head->next == head;
}


/***************************************************************
 * @description: 
 * @param {list} *head [in/out]:  
 * @param {list_t} *node [in/out]:  
 * @return {*}
***************************************************************/
STATIC_INLINE void list_splice(list_t *head,list_t *node)
{
    if(!list_empty(head))
        __list_splice(head,node);
}


/***************************************************************
 * @description: 
 * @param {list} *head [in/out]:  
 * @param {list_t} *node [in/out]:  
 * @return {*}
***************************************************************/
STATIC_INLINE void list_splice_init(list_t *head,list_t *node)
{
    if(!list_empty(head))
    {
        __list_splice(head,node);
        INIT_LIST_HEAD(head);
    }
        
}


/***************************************************************
 * @description: 访问链表成员所在的结构体
 * @mptr: 链表成员的地址
 * @stype: 结构体的类型
 * @member:结构体的成员名
***************************************************************/  
#define list_entry(mptr,stype,member) \
    container_of(mptr,stype,member)

/***************************************************************
 * @description: 从前向后遍历链表
 * @pos: 一个链表指针
 * @head: 链表头节点地址
***************************************************************/
#define list_for_each(pos,head) \
    for( (pos)=(head)->next;(pos)!=(head);(pos)=(pos)->next)

#define list_for_each_safe(pos,n,head) \
    for( (pos)=(head)->next,(n)=(pos)->next; \
        (pos)!=(head); \
        (pos)=(n),(n)=(pos)->next)

/***************************************************************
 * @description: 从后向前遍历链表
 * @pos: 一个链表指针
 * @head: 链表头节点地址
***************************************************************/
#define list_for_each_prev(pos,head) \
    for( (pos)=(head)->prev;(pos)!=(head);(pos)=(pos)->prev)


/***************************************************************
 * @description: 遍历由链表成员串起来的结构体
 * @pos: 一个结构体指针
 * @head: 链表头节点地址
 * @stype: 结构体类型,如tcb_t
 * @member: 结构体中链表成员的名字
***************************************************************/
#define list_for_each_entry(pos,head,stype,member) \
    for( (pos)=list_entry((head)->next,stype,member); \
        &(pos->member)!=(head); \
        (pos)=list_entry((pos->member.next),stype,member))

#define list_for_each_entry_safe(pos,n,head,stype,member) \
    for( (pos)=list_entry((head)->next,stype,member), \
        (n)=list_entry((pos->member.next),stype,member); \
        &(pos->member)!=(head); \
        (pos)=(n),(n)=list_entry((n->member.next),stype,member))

#define list_for_each_entry_prev(pos,head,stype,member) \
    for( (pos)=list_entry((head)->prev,stype,member); \
        &(pos->member)!=(head); \
        (pos)=list_entry((pos->member.prev),stype,member))        

#endif