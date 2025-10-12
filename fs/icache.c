/**
 * @FilePath: /vboot/home/wei/os/ZZZ-OS/fs/icache.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 00:51:46
 * @LastEditTime: 2025-10-10 01:09:57
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "malloc.h"
#include "check.h"
#include "lru.h"
#include "spinlock.h"
#include "string.h"
#include "time.h"
#include "container_of.h"
#include "icache.h"

static void inode_lock(struct inode *inode) 
{
   spin_lock(&inode->i_lock);
}

static void inode_unlock(struct inode *inode) 
{
    spin_unlock(&inode->i_lock);
}

static hval_t inode_self_page_cache_hash(const struct hlist_node* node)
{
    struct page *page = container_of(node, struct page, p_lru_cache_node);
    return page->index % 64;
}

static int inode_self_page_cache_compare(const struct hlist_node* node_a, const struct hlist_node* node_b)
{
    struct page *a = container_of(node_a, struct page, p_lru_cache_node);
    struct page *b = container_of(node_b, struct page, p_lru_cache_node);
    return a->index - b->index;
}

int destroy_inode(struct inode *inode)
{
    CHECK(inode != NULL, "vfs_free_inode: inode is NULL", return -1;);
    CHECK(inode->i_refcount > 0, "vfs_free_inode: inode is still in use", return -1;);

    if (inode->dirty && inode->i_sb->s_ops->write_inode) 
    {
        inode->i_sb->s_ops->write_inode(inode);
    }

    hashtable_destroy(inode->i_mapping->page_cache);
    free(inode->i_mapping);
    free(inode);
    free(inode->i_private);
    return 0;
}

struct inode* create_inode(struct superblock *sb)
{
    CHECK(sb != NULL, "vfs_alloc_inode: sb is NULL", return NULL;);

    struct inode *new_inode_ret = malloc(sizeof(struct inode));
    CHECK(new_inode_ret != NULL, "Memory allocation for inode failed", return NULL;);

    memset(new_inode_ret, 0, sizeof(struct inode));

    u32 current_time = 0;
    // 初始化时间戳，这里用的是当前北京时间的时间戳，但是这里的时间可能会被实际读取的inode时间覆盖，这里先初始化一下。
    new_inode_ret->i_atime.tv_sec = current_time;
    new_inode_ret->i_atime.tv_nsec = 0;
    new_inode_ret->i_mtime.tv_sec = current_time;
    new_inode_ret->i_mtime.tv_nsec = 0;
    new_inode_ret->i_ctime.tv_sec = current_time;
    new_inode_ret->i_ctime.tv_nsec = 0;

    new_inode_ret->i_sb = sb;
    new_inode_ret->i_lock.lock = 0;

    lru_node_init(&new_inode_ret->i_lru_cache_node);

    new_inode_ret->i_mapping = malloc(sizeof(struct address_space));
    new_inode_ret->i_mapping->host = new_inode_ret;
    new_inode_ret->i_mapping->page_cache = hashtable_init(64, inode_self_page_cache_hash, inode_self_page_cache_compare);

    return new_inode_ret;
}

struct inode * inew(struct superblock *sb)
{
    struct inode *new_inode_ret = create_inode(sb);
    CHECK(new_inode_ret != NULL, "vfs_new_inode: Failed to create new inode", return NULL;);
    
    // 调用文件系统的create_private_inode函数初始化私有inode数据
    if(sb->s_ops->new_private_inode(new_inode_ret) >= 0)
    {
        return new_inode_ret;
    }
    
    destroy_inode(new_inode_ret);
    return NULL;

}



struct lru_cache *global_inode_cache = NULL;

static hval_t icache_lru_hash(const struct hlist_node *node) 
{
    struct inode *inode = container_of(node, struct inode, i_lru_cache_node);
    uintptr_t sb_ptr = (uintptr_t)inode->i_sb;
    
    // 使用黄金比例相关的魔数
    const u32 golden_ratio = 0x9E3779B9U;
    
    // 混合两个值
    hval_t hash = (hval_t)sb_ptr * golden_ratio;
    hash ^= (hval_t)inode->i_ino * golden_ratio;
    
    // 再次混合以增强分布
    hash = hash ^ (hash >> 16);
    
    return hash;
}

static int icache_lru_compare(const struct hlist_node *a, const struct hlist_node *b) 
{
    struct inode *inode_a = container_of(a, struct inode, i_lru_cache_node);
    struct inode *inode_b = container_of(b, struct inode, i_lru_cache_node);
    
    if (inode_a->i_sb == inode_b->i_sb && inode_a->i_ino == inode_b->i_ino) 
    {
        return 0; // 相等
    }
    return 1; // 不相等
}

static int icache_lru_free(struct lru_node *node)
{
    CHECK(node != NULL, "vfs_lru_free: node is NULL", return -1;);

    struct inode *inode = container_of(node, struct inode, i_lru_cache_node);
    return destroy_inode(inode);
}

static int icache_lru_sync(struct lru_node *node)
{
    CHECK(node != NULL, "icache_sync_func: node is NULL", return -1;);

    struct inode *inode = container_of(node, struct inode, i_lru_cache_node);
    inode_lock(inode);
    if (inode->dirty && inode->i_sb->s_ops->write_inode) 
    {
        inode->i_sb->s_ops->write_inode(inode);
        inode->dirty = false;
    }
    inode_unlock(inode);
    return 0;
}

int icache_init()
{
    global_inode_cache = lru_init(128, icache_lru_free,icache_lru_sync,icache_lru_hash, icache_lru_compare);
    CHECK(global_inode_cache != NULL, "Failed to create inode LRU cache", return -1;);
    return 0;
}

void icache_destroy()
{
    lru_destroy(global_inode_cache);
}

struct inode *iget(struct superblock *sb, ino_t ino) 
{
    CHECK(sb != NULL, "iget: sb is NULL", return NULL;);
   
    // 构造一个用来查找的inode
    struct inode temp_inode;
    temp_inode.i_sb = sb;
    temp_inode.i_ino = ino;

    struct inode *new_inode_ret = NULL;
    // 先从缓存中查找
    struct lru_node *found_node = lru_lookup(global_inode_cache, &temp_inode.i_lru_cache_node);
    if (found_node) 
    {
        // 找到，返回缓存中的inode
        new_inode_ret = container_of(found_node, struct inode, i_lru_cache_node);
        inode_lock(new_inode_ret);
        new_inode_ret->i_refcount++;
        lru_get(found_node); // 从淘汰链表中移除，防止被回收
        inode_unlock(new_inode_ret);
        return new_inode_ret; 
    }
    // 没找到，从磁盘读取
    new_inode_ret = create_inode(sb);
    CHECK(new_inode_ret != NULL, "Memory allocation for inode failed", return NULL;);
    new_inode_ret->i_private = sb->s_ops->create_private_inode(new_inode_ret);
    new_inode_ret->i_ino = ino;

    if(sb->s_ops->read_inode(new_inode_ret)>=0)
    {
        inode_lock(new_inode_ret);
        new_inode_ret->i_refcount = 1;
        lru_insert(global_inode_cache, &new_inode_ret->i_lru_cache_node);
        lru_get(found_node);
        inode_unlock(new_inode_ret);
        return new_inode_ret;
    }
    
    destroy_inode(new_inode_ret);
    return NULL;
}

int iput(struct inode *inode) 
{
    CHECK(inode != NULL, "vfs_input:inode is NULL", return -1;); 
    inode_lock(inode);
    
    if (inode->i_refcount > 0)
    {
        inode->i_refcount--;
        lru_update(global_inode_cache, &inode->i_lru_cache_node);
        inode_unlock(inode);
        return 0;
    }

    lru_put(global_inode_cache, &inode->i_lru_cache_node);
    
    inode_unlock(inode);
    return 0;
}

int icache_sync(struct inode *inode)
{
    CHECK(inode != NULL, "icache_sync: inode is NULL", return -1;);
    return icache_lru_sync(&inode->i_lru_cache_node);
}

int icache_sync_all()
{
    CHECK(global_inode_cache != NULL, "icache_sync_all: global_inode_cache is NULL", return -1;);
    lru_walk(global_inode_cache, icache_lru_sync);
    return 0;
}
