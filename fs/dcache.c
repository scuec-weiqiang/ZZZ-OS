#include "types.h"
#include "container_of.h"
#include "lru.h"
#include "vfs_types.h"
#include "check.h"
#include "string.h"
#include "malloc.h"
#include "list.h"

struct dentry *create_dentry(const char *name)
{
    CHECK(name != NULL, "vfs_dentry_create: Invalid name", return NULL;);

    struct dentry *dentry = malloc(sizeof(struct dentry));
    CHECK(dentry != NULL, "vfs_dentry_create: Memory allocation failed", return NULL;);

    memset(dentry, 0, sizeof(struct dentry));

    u32 name_len = strlen(name);
    dentry->name.name = malloc(name_len + 1);
    CHECK(dentry->name.name != NULL, "vfs_dentry_create: Memory allocation for name failed",
          free(dentry);
          return NULL;);

    memcpy(dentry->name.name, name, name_len);
    dentry->name.name[name_len] = '\0'; // 确保字符串以null结尾
    dentry->name.len = name_len;

    INIT_LIST_HEAD(&dentry->d_childs);
    INIT_LIST_HEAD(&dentry->d_subdirs);

    lru_node_init(&dentry->d_lru_cache_node);

    return dentry;
}

int vfs_destroy_dentry(struct dentry *dentry)
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

static hval_t dentry_cache_hash(const struct hlist_node *node)
{
#define FNV_PRIME 16777619u
#define FNV_OFFSET 2166136261u

    struct dentry *dentry = container_of(node, struct dentry, d_lru_cache_node);
    ino_t parent_ino = dentry->d_parent ? dentry->d_parent->d_inode->i_ino : 0;

    u64 h = FNV_OFFSET ^ parent_ino;

    for (size_t i = 0; i < dentry->name.len; i++)
    {
        h ^= (unsigned char)dentry->name.name[i];
        h *= FNV_PRIME;
    }

    return h;
}

static int dentry_cache_compare(const struct hlist_node *node_a, const struct hlist_node *node_b)
{
    struct dentry *a = container_of(node_a, struct dentry, d_lru_cache_node);
    struct dentry *b = container_of(node_b, struct dentry, d_lru_cache_node);

    if (a->d_parent == b->d_parent && a->name.len == b->name.len && strcmp(a->name.name, b->name.name) == 0)
    {
        return 0; // 相等
    }
    return 1; // 不相等
}

static int dentry_cache_free(struct lru_node *node)
{
    vfs_destroy_dentry(container_of(node, struct dentry, d_lru_cache_node));
    return 0;
}

struct lru_cache *global_dentry_cache = NULL;
static void dentry_lock(struct dentry *dentry)
{
    spin_lock(&dentry->d_lock);
}

static void dentry_unlock(struct dentry *dentry)
{
    spin_unlock(&dentry->d_lock);
}

int dcache_init()
{
    global_dentry_cache = lru_init(128, dentry_cache_free, NULL, dentry_cache_hash, dentry_cache_compare);
    CHECK(global_dentry_cache != NULL, "Failed to create dentry LRU cache", return -1;);
    return 0;
}

void dcache_destroy()
{
    lru_destroy(global_dentry_cache);
}

struct dentry *dnew(struct dentry *parent, const char *name, struct inode *inode)
{
    CHECK(name != NULL, "vfs_new_dentry: name is NULL", return NULL;);
    CHECK(parent == NULL || parent->d_inode != NULL, "vfs_new_dentry: Invalid parent", return NULL;);

    struct dentry *dentry = create_dentry(name);
    CHECK(dentry != NULL, "vfs_new_dentry: Failed to create dentry", return NULL;);

    dentry_lock(dentry);
    dentry->d_parent = parent;
    dentry->d_refcount = 0;
    list_add(&parent->d_subdirs, &dentry->d_childs);

    if (inode)
    {
        dentry->d_inode = inode;
    }
    else
    {
        dentry->d_inode = NULL;
        // dentry->d_flags |= DCACHE_NEGATIVE;
    }
    dentry_unlock(dentry);

    return dentry;
}

struct dentry *dget(struct dentry *parent, const char *name)
{
    CHECK(name != NULL, "dget: name is NULL", return NULL;);
    CHECK(parent == NULL || parent->d_inode != NULL, "dget: Invalid parent", return NULL;);

    // 构造一个临时的dentry用于查找
    struct dentry temp_dentry;
    temp_dentry.d_parent = parent;
    temp_dentry.name.name = (char *)name; // 强制转换为非const，因为我们只是用它来计算哈希
    temp_dentry.name.len = strlen(name);
    lru_node_init(&temp_dentry.d_lru_cache_node);

    // 在缓存中查找
    struct lru_node *found_node = lru_lookup(global_dentry_cache, &temp_dentry.d_lru_cache_node);
    if (found_node)
    {
        struct dentry *found_dentry = container_of(found_node, struct dentry, d_lru_cache_node);
        dentry_lock(found_dentry);
        lru_get(found_node); // 从淘汰链表中移除，防止被回收
        found_dentry->d_refcount++;
        dentry_unlock(found_dentry);
        return found_dentry;
    }

    // 没找到，创建一个新的dentry并插入缓存
    struct dentry *new_dentry_inode = dnew(parent, name, NULL);
    dentry_lock(new_dentry_inode);
    new_dentry_inode->d_refcount = 1;
    parent->d_inode->i_ops->lookup(parent->d_inode, new_dentry_inode);
    lru_insert(global_dentry_cache, &new_dentry_inode->d_lru_cache_node);
    lru_get(found_node); // 从淘汰链表中移除，防止被回收
    dentry_unlock(new_dentry_inode);
    return new_dentry_inode;
}

int dput(struct dentry *dentry)
{
    CHECK(dentry != NULL, "dput: dentry is NULL", return -1;);

    dentry_lock(dentry);
    if (dentry->d_refcount > 0)
    {
        dentry->d_refcount--;
        lru_update(global_dentry_cache, &dentry->d_lru_cache_node);
        dentry_unlock(dentry);
        return 0;
    }
    lru_put(global_dentry_cache, &dentry->d_lru_cache_node);
    dentry_unlock(dentry);

    return 0;
}
