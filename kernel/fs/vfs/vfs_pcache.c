/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_pcache.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 16:25:09
 * @LastEditTime: 2025-09-07 22:41:23
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "types.h"
#include "container_of.h"
#include "lru.h"
#include "spinlock.h"
#include "vfs_types.h"
#include "check.h"


lru_cache_t *global_page_cache = NULL;


static void lock(vfs_page_t *page) 
{
   spin_lock(&(page->lock));
}

static void unlock(vfs_page_t *page) 
{
    spin_unlock(&(page->lock));
}

static vfs_page_t *vfs_create_page(vfs_inode_t *inode, pgoff_t index) 
{
    vfs_page_t *p = malloc(sizeof(vfs_page_t));
    CHECK(p != NULL, "Memory allocation for page failed", return NULL;);
    memset(p, 0, sizeof(vfs_page_t));
    p->data = malloc(VFS_PAGE_SIZE);
    CHECK(p->data != NULL, "Memory allocation for page data failed", free(p); return NULL;);

    p->lock.lock = 0;

    lock(p);
    p->inode = inode;
    p->index = index;
    p->uptodate = false;
    p->dirty = false;
    p->under_io = true; //占位态, 表示正在加载
    lru_node_init(&p->p_lru_cache_node);
    hlist_node_init(&p->self_cache_node);
    unlock(p);

    return p;
}

static void *vfs_destory_page(vfs_page_t *page)
{
    if (page->data)
        free(page->data);
    free(page);
    return 0;
}


static hval_t vfs_global_page_lru_hash(const hlist_node_t *node) 
{
    vfs_page_t *page = container_of(node, vfs_page_t, p_lru_cache_node);
    uintptr_t sb_ptr = (uintptr_t)page->inode;
  
    // 使用黄金比例相关的魔数
    const uint32_t golden_ratio = 0x9E3779B9U;
    
    // 混合两个值
    hval_t hash = (hval_t)sb_ptr * golden_ratio;
    hash ^= (hval_t)page->index * golden_ratio;
    
    // 再次混合以增强分布
    hash = hash ^ (hash >> 16);
    
    return hash;
}
static int64_t vfs_global_page_lru_compare(const hlist_node_t *a, const hlist_node_t *b) 
{
    vfs_page_t *page_a = container_of(a, vfs_page_t, p_lru_cache_node);
    vfs_page_t *page_b = container_of(b, vfs_page_t, p_lru_cache_node);
    
    if (page_a->inode == page_b->inode && page_a->index == page_b->index) 
    {
        return 0; // 相等
    }
    return 1; // 不相等
}
static int64_t vfs_global_page_lru_free(lru_node_t *node)
{
    CHECK(node != NULL, "vfs_lru_free: node is NULL", return -1;);

    vfs_page_t *page = container_of(node, vfs_page_t, p_lru_cache_node);
    return vfs_destory_page(page);
}


int64_t vfs_pcache_init()
{
    global_page_cache = lru_init(128, vfs_global_page_lru_free,vfs_global_page_lru_hash, vfs_global_page_lru_compare);
    CHECK(global_page_cache != NULL, "Failed to create page LRU cache", return -1;);
    return 0;
}

void vfs_pcache_destory()
{
    lru_destroy(global_page_cache);
}



vfs_page_t *vfs_pget(vfs_inode_t *inode, uint32_t index)
{
    vfs_page_t page;
    page.inode = inode;
    page.index = index;
    page.lock.lock = 0;

    vfs_page_t *found_page = NULL;
    // 1. 在 page cache 里查找 (基于 inode + index 的哈希)
    hlist_node_t *node = hashtable_lookup(inode->i_mapping->page_cache,&page.self_cache_node);
    if(node)
    {
        found_page = container_of(node,vfs_page_t,self_cache_node);
        return found_page;
    }

    // 2. 没有的话分配新页
    found_page = vfs_create_page(inode, index);

    // 3. 调用文件系统的 readpage，把数据读进来
    inode->i_mapping->a_ops->readpage(found_page);

    // 4. 插入缓存
    hashtable_insert(inode->i_mapping->page_cache,&found_page->self_cache_node);
    lru_hash_insert(global_page_cache,&found_page->p_lru_cache_node);
    lru_ref(global_page_cache,&found_page->p_lru_cache_node); // 引用计数+1，表示正在使用

    return found_page;
}

int64_t vfs_pput(vfs_page_t *page)
{
    CHECK(page != NULL, "vfs_pput: page is NULL", return -1;);
    lock(page);
    lru_unref(global_page_cache,&page->p_lru_cache_node); // 引用计数-1
    if(page->p_lru_cache_node.ref_count == 0)
    {
        hashtable_remove(page->inode->i_mapping->page_cache,&page->self_cache_node);
    } 
    unlock(page);
    return 0;
}

int64_t vfs_pcache_sync_func(lru_cache_t *cache,lru_node_t *node)
{
    vfs_page_t *page = container_of(node, vfs_page_t, p_lru_cache_node);
    lock(page);
    if (page->dirty && page->inode->i_mapping->a_ops->writepage) 
    {
        int64_t ret = page->inode->i_mapping->a_ops->writepage(page);
        CHECK(ret >= 0, "vfs_pcache_sync: writepage failed", );
        page->dirty = false;
    }
    unlock(page);
    return 0;
}

int64_t vfs_pcache_sync()
{
    CHECK(global_page_cache != NULL, "vfs_pcache_sync: global_page_cache is NULL", return -1;);
    lru_walk(global_page_cache, vfs_pcache_sync_func);
    return 0;
}