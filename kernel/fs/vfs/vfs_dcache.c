#include "types.h"
#include "container_of.h"
#include "lru.h"
#include "vfs_types.h"
#include "check.h"
#include "string.h"
#include "page_alloc.h"
#include "list.h"


vfs_dentry_t* vfs_create_dentry(const char *name)
{
    CHECK(name != NULL, "vfs_dentry_create: Invalid name", return NULL;);

    vfs_dentry_t *dentry = malloc(sizeof(vfs_dentry_t));
    CHECK(dentry != NULL, "vfs_dentry_create: Memory allocation failed", return NULL;);

    memset(dentry, 0, sizeof(vfs_dentry_t));

    uint32_t name_len = strlen(name);
    dentry->name.name = malloc(name_len + 1);
    CHECK(dentry->name.name != NULL, "vfs_dentry_create: Memory allocation for name failed", 
          free(dentry); return NULL;);
 
    memcpy(dentry->name.name, name, name_len);
    dentry->name.name[name_len] = '\0'; // 确保字符串以null结尾
    dentry->name.len = name_len;

    INIT_LIST_HEAD(&dentry->d_childs);
    INIT_LIST_HEAD(&dentry->d_subdirs);

    lru_node_init(&dentry->d_lru_cache_node);
    
    return dentry;
}

int64_t vfs_destroy_dentry(vfs_dentry_t *dentry)
{
    CHECK(dentry != NULL, "vfs_destroy_dentry: dentry is NULL", return -1;);

    if (dentry->d_parent) 
    {
        list_del(&dentry->d_childs);
    }
    free(dentry->name.name);
    free(dentry);
    return 0;
}



static hval_t dentry_cache_hash(const hlist_node_t* node)
{
    #define FNV_PRIME 16777619u
    #define FNV_OFFSET 2166136261u

    vfs_dentry_t *dentry = container_of(node, vfs_dentry_t, d_lru_cache_node);
    vfs_ino_t parent_ino = dentry->d_parent ? dentry->d_parent->d_inode->i_ino : 0;

    uint64_t h = FNV_OFFSET ^ parent_ino;

    for (size_t i = 0; i < dentry->name.len; i++) 
    {
        h ^= (unsigned char)dentry->name.name[i];
        h *= FNV_PRIME;
    }

    return h;
}

static int64_t dentry_cache_compare(const hlist_node_t* node_a, const hlist_node_t* node_b)
{
    vfs_dentry_t *a = container_of(node_a, vfs_dentry_t, d_lru_cache_node);
    vfs_dentry_t *b = container_of(node_b, vfs_dentry_t, d_lru_cache_node);

    if (a->d_parent == b->d_parent && a->name.len == b->name.len && strcmp(a->name.name, b->name.name) == 0)
    {
        return 0; // 相等
    }
    return 1; // 不相等
}

static int64_t dentry_cache_free(hlist_node_t* node)
{
    vfs_destroy_dentry(container_of(node, vfs_dentry_t, d_lru_cache_node));
}


lru_cache_t *global_dentry_cache = NULL;
static void dentry_lock(vfs_dentry_t *dentry) 
{
   spin_lock(&dentry->d_lock);
}

static void dentry_unlock(vfs_dentry_t *dentry) 
{
    spin_unlock(&dentry->d_lock);
}

int64_t vfs_dcache_init()
{
    global_dentry_cache = lru_init(128, vfs_destroy_dentry, dentry_cache_hash, dentry_cache_compare);
    CHECK(global_dentry_cache != NULL, "Failed to create dentry LRU cache", return -1;);
    return 0;
}

void vfs_dcache_destory()
{
    lru_destroy(global_dentry_cache);
}


vfs_dentry_t* vfs_dnew(vfs_dentry_t *parent, const char *name, vfs_inode_t *inode)
{
    CHECK(name != NULL, "vfs_new_dentry: name is NULL", return NULL;);
    CHECK(parent == NULL || parent->d_inode != NULL, "vfs_new_dentry: Invalid parent", return NULL;);

    vfs_dentry_t *dentry = vfs_create_dentry(name);
    CHECK(dentry != NULL, "vfs_new_dentry: Failed to create dentry", return NULL;);

    dentry_lock(dentry);
    dentry->d_parent = parent;
    list_add(&parent->d_subdirs, &dentry->d_childs);
    
    if(inode)
    {
        dentry->d_inode = inode;
    }
    else
    {
        dentry->d_inode = NULL;
        // dentry->d_flags |= DCACHE_NEGATIVE;
    }
    lru_ref(global_dentry_cache, &dentry->d_lru_cache_node);
    lru_hash_insert(global_dentry_cache, &dentry->d_lru_cache_node);
    dentry_unlock(dentry);

    return dentry;
}

vfs_dentry_t* vfs_dget(vfs_dentry_t *parent, const char *name)
{
    CHECK(name != NULL, "vfs_dget: name is NULL", return NULL;);
    CHECK(parent == NULL || parent->d_inode != NULL, "vfs_dget: Invalid parent", return NULL;);

    // 构造一个临时的dentry用于查找
    vfs_dentry_t temp_dentry;
    temp_dentry.d_parent = parent;
    temp_dentry.name.name = (char*)name; // 强制转换为非const，因为我们只是用它来计算哈希
    temp_dentry.name.len = strlen(name);
    lru_node_init(&temp_dentry.d_lru_cache_node);

    // 在缓存中查找
    lru_node_t *found_node = lru_hash_lookup(global_dentry_cache, &temp_dentry.d_lru_cache_node);
    if (found_node) 
    {
        vfs_dentry_t *found_dentry = container_of(found_node, vfs_dentry_t, d_lru_cache_node);
        dentry_lock(found_dentry);
        lru_ref(global_dentry_cache, &found_dentry->d_lru_cache_node);
        dentry_unlock(found_dentry);
        return found_dentry;
    }
    
    // 没找到，创建一个新的dentry并插入缓存
    vfs_dentry_t* new_dentry_inode = vfs_dnew(parent, name, NULL); 
    parent->d_inode->i_ops->lookup(parent->d_inode, new_dentry_inode);
    return new_dentry_inode;
}

int64_t vfs_dput(vfs_dentry_t *dentry)
{
    CHECK(dentry != NULL, "vfs_dput: dentry is NULL", return -1;);
    dentry_lock(dentry);
    lru_unref(global_dentry_cache, &dentry->d_lru_cache_node);
    dentry_unlock(dentry);
    return ;
}