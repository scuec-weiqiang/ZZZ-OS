/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_icache.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-28 00:51:46
 * @LastEditTime: 2025-09-04 15:46:59
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "page_alloc.h"
#include "check.h"
#include "lru.h"
#include "spinlock.h"
#include "string.h"
#include "time.h"
#include "container_of.h"
#include "vfs_icache.h"


static hval_t inode_self_page_cache_hash(const hlist_node_t* node)
{
    vfs_page_t *page = container_of(node, vfs_page_t, p_lru_cache_node);
    return page->index % 64;
}

static int64_t inode_self_page_cache_compare(const hlist_node_t* node_a, const hlist_node_t* node_b)
{
    vfs_page_t *a = container_of(node_a, vfs_page_t, p_lru_cache_node);
    vfs_page_t *b = container_of(node_b, vfs_page_t, p_lru_cache_node);
    return a->index - b->index;
}


int64_t vfs_destory_inode(vfs_inode_t *inode)
{
    CHECK(inode != NULL, "vfs_free_inode: inode is NULL", return -1;);
    CHECK(inode->i_count > 0, "vfs_free_inode: inode is still in use", return -1;);

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

vfs_inode_t* vfs_create_inode(vfs_superblock_t *sb)
{
    CHECK(sb != NULL, "vfs_alloc_inode: sb is NULL", return NULL;);

    vfs_inode_t *new_inode_ret = malloc(sizeof(vfs_inode_t));
    CHECK(new_inode_ret != NULL, "Memory allocation for inode failed", return NULL;);

    memset(new_inode_ret, 0, sizeof(vfs_inode_t));

    uint32_t current_time = get_current_unix_timestamp(UTC8);
    // 初始化时间戳，这里用的是当前北京时间的时间戳，但是这里的时间可能会被实际读取的inode时间覆盖，这里先初始化一下。
    new_inode_ret->i_atime.tv_sec = current_time;
    new_inode_ret->i_atime.tv_nsec = 0;
    new_inode_ret->i_mtime.tv_sec = current_time;
    new_inode_ret->i_mtime.tv_nsec = 0;
    new_inode_ret->i_ctime.tv_sec = current_time;
    new_inode_ret->i_ctime.tv_nsec = 0;

    new_inode_ret->i_sb = sb;
    new_inode_ret->i_count = 0;
    new_inode_ret->i_lock.lock = 0;
    // 初始化LRU节点
    hlist_node_init(&new_inode_ret->i_lru_cache_node.hnode);
    INIT_LIST_HEAD(&new_inode_ret->i_lru_cache_node.lnode);

    new_inode_ret->i_mapping = malloc(sizeof(vfs_address_space_t));
    new_inode_ret->i_mapping->host = new_inode_ret;
    new_inode_ret->i_mapping->page_cache = hashtable_init(64, inode_self_page_cache_hash, inode_self_page_cache_compare);

    return new_inode_ret;
}


vfs_inode_t* vfs_new_inode(vfs_superblock_t *sb)
{
    vfs_inode_t *new_inode_ret = vfs_create_inode(sb);
    CHECK(new_inode_ret != NULL, "vfs_new_inode: Failed to create new inode", return NULL;);
    
    // 调用文件系统的create_private_inode函数初始化私有inode数据
    if(0 == sb->s_ops->new_private_inode(new_inode_ret))
    {
        vfs_iput(new_inode_ret);
        return new_inode_ret;
    }
    
    vfs_destory_inode(new_inode_ret);
    return NULL;

}





lru_cache_t *global_inode_cache = NULL;

static hval_t vfs_inode_lru_hash(const hlist_node_t *node) 
{
    vfs_inode_t *vfs_inode = container_of(node, vfs_inode_t, i_lru_cache_node);
    uintptr_t sb_ptr = (uintptr_t)vfs_inode->i_sb;
    
    // 使用黄金比例相关的魔数
    const uint32_t golden_ratio = 0x9E3779B9U;
    
    // 混合两个值
    hval_t hash = (hval_t)sb_ptr * golden_ratio;
    hash ^= (hval_t)vfs_inode->i_ino * golden_ratio;
    
    // 再次混合以增强分布
    hash = hash ^ (hash >> 16);
    
    return hash;
}

static int64_t vfs_inode_lru_compare(const hlist_node_t *a, const hlist_node_t *b) 
{
    vfs_inode_t *inode_a = container_of(a, vfs_inode_t, i_lru_cache_node);
    vfs_inode_t *inode_b = container_of(b, vfs_inode_t, i_lru_cache_node);
    
    if (inode_a->i_sb == inode_b->i_sb && inode_a->i_ino == inode_b->i_ino) 
    {
        return 0; // 相等
    }
    return 1; // 不相等
}

static int64_t vfs_inode_lru_free(lru_node_t *node)
{
    CHECK(node != NULL, "vfs_lru_free: node is NULL", return -1;);

    vfs_inode_t *inode = container_of(node, vfs_inode_t, i_lru_cache_node);
    return vfs_destory_inode(inode);
}

int64_t vfs_icache_init()
{
    global_inode_cache = lru_cache_init(128, vfs_inode_lru_free,vfs_inode_lru_hash, vfs_inode_lru_compare);
    CHECK(global_inode_cache != NULL, "Failed to create inode LRU cache", return -1;);
    return 0;
}

void vfs_icache_destory()
{
    lru_cache_destroy(global_inode_cache);
}

static void inode_lock(vfs_inode_t *inode) 
{
   spin_lock(&inode->i_lock);
}

static void inode_unlock(vfs_inode_t *inode) 
{
    spin_unlock(&inode->i_lock);
}


vfs_inode_t *vfs_iget(vfs_superblock_t *sb, vfs_ino_t ino) 
{
    CHECK(sb != NULL, "vfs_iget: sb is NULL", return NULL;);
   
    // 构造一个用来查找的inode
    vfs_inode_t temp_inode;
    temp_inode.i_sb = sb;
    temp_inode.i_ino = ino;

    vfs_inode_t *new_inode_ret = NULL;
    // 先从缓存中查找
    lru_node_t *found_node = lru_cache_get(global_inode_cache, &temp_inode.i_lru_cache_node);
    if (found_node) 
    {
        // 找到，返回缓存中的inode
        new_inode_ret = container_of(found_node, vfs_inode_t, i_lru_cache_node);
        inode_lock(new_inode_ret);
        new_inode_ret->i_count++;
        inode_unlock(new_inode_ret);
        return new_inode_ret; 
    }
    // 没找到，从磁盘读取
    new_inode_ret = vfs_create_inode(sb);
    CHECK(new_inode_ret != NULL, "Memory allocation for inode failed", return NULL;);
    new_inode_ret->i_private = sb->s_ops->create_private_inode(new_inode_ret);
    new_inode_ret->i_ino = ino;

    if(sb->s_ops->read_inode(new_inode_ret)>=0)
    {
        inode_lock(new_inode_ret);
        new_inode_ret->i_count++;
        inode_unlock(new_inode_ret);
        return new_inode_ret;
    }
    
    vfs_destory_inode(new_inode_ret);
    return NULL;
    
    
}

int64_t vfs_iput(vfs_inode_t *inode) 
{
    CHECK(inode != NULL, "vfs_input:inode is NULL", return -1;); 
    inode_lock(inode);
    if(inode->i_count > 0)
    {
        inode->i_count--;
    }
    if(inode->i_count == 0)
    {
        lru_cache_insert(global_inode_cache, &inode->i_lru_cache_node);
    }
    inode_unlock(inode);

    return 0;
}

int64_t vfs_icache_sync()
{
    CHECK(global_inode_cache != NULL, "vfs_icache_sync: global_inode_cache is NULL", return -1;);
    lru_node_t *node = NULL;
    // 遍历全局inode缓存，写回所有脏inode
    list_for_each_entry(node, &global_inode_cache->lhead,lru_node_t, lnode) 
    {
        vfs_inode_t *inode = container_of(node, vfs_inode_t, i_lru_cache_node);
        inode_lock(inode);
        if (inode->dirty && inode->i_sb->s_ops->write_inode) 
        {
            int64_t ret = inode->i_sb->s_ops->write_inode(inode);
            list_mov_tail(&global_inode_cache->lhead, &inode->i_lru_cache_node.lnode);
            CHECK(ret >= 0, "vfs_icache_sync: write_inode failed", inode_unlock(inode); return -1;);
            inode->dirty = false;
        }
        inode_unlock(inode);
    }
    return 0;
}
