#ifndef list_H
#define list_H

#include "types.h"

/***************************************************************
 * @description: 返回结构体成员的偏移地址
 * @stype: 结构体的类型
 * @member:结构体的一个成员名
***************************************************************/
#define offsetof(stype,member)  \
    ((size_t)(&(((stype*)0)->member))) \
            
/***************************************************************
 * @description: 返回某个成员所在结构体的首地址
 * @mptr: 成员的地址
 * @stype: 结构体的类型
 * @member:结构体的成员名
***************************************************************/   
// 现在我有一个b的地址，我想知道这个b所在的A结构体的首地址，就可以用这个宏
#define container_of(mptr,stype,member) \
    ((stype*)((size_t)mptr-offsetof(stype,member))) \


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
 * @param {list_t} *node [in]:  需要插入的节点
 * @param {list_t} *prev [in]:  插入位置的前一个节点
 * @param {list_t} *next [in]:  插入位置的后一个节点
 * @return {*}
***************************************************************/
__SELF __INLINE void __list_add(list_t *node,list_t *prev,list_t *next)
{   
    prev->next = node;
    node->prev = prev;
    next->prev = node;
    node->next = next;
}

/***************************************************************
 * @description: 卸载两个节点之间的节点
 * @param {list_t} *prev [in]:  卸载位置的前一个节点 
 * @param {list_t} *next [in]:  卸载位置的后一个节点
 * @return {*}
***************************************************************/
__SELF __INLINE void __list_del(list_t *prev,list_t *next)
{
    prev->next = next;
    next->prev = prev;
}


/***************************************************************
 * @description: 将链表<head>合并到某一链表节点<node>的后面
 * @param {list_t} *head [in]:  被合并的链表的头
 * @param {list_t} *node [in]:  需要合并位置的节点
 * @return {*}
***************************************************************/
__SELF __INLINE void __list_splice(list_t *head,list_t *node)
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
 * @param {list_t} *node [in]:  需要插入的新节点
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
    // node->prev = NULL_PTR;
    // node->next = NULL_PTR;
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
 * @description: 由链表节点访问其所在的结构体
***************************************************************/
#define list_entry(mptr,stype,member) \
    container_of(mptr,stype,member)

/***************************************************************
 * @description: 从前向后遍历链表
***************************************************************/
#define list_for_each(pos,head) \
    for( (pos)=(head)->next;(pos)!=(head);(pos)=(pos)->next)

/***************************************************************
 * @description: 从后向前遍历链表
***************************************************************/
#define list_for_each_prev(pos,head) \
    for( (pos)=(head)->prev;(pos)!=(head);(pos)=(pos)->prev)


#define list_for_each_entry(pos,head,stype,member) \
    for( (pos)=list_entry((head)->next,stype,member); \
        &(pos->member)!=(head); \
        (pos)=list_entry((pos->member.next),stype,member))


#define list_for_each_entry_prev(pos,head,stype,member) \
    for( (pos)=list_entry((head)->prev,stype,member); \
        &(pos->member)!=(head); \
        (pos)=list_entry((pos->member.prev),stype,member))        

#endif