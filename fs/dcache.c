#include <fs/dcache.h>
#include <fs/inode.h>
#include <os/check.h>
#include <os/container_of.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/lru.h>
#include <os/string.h>
#include <os/string.h>

static struct lru_cache *g_dcache;

static inline void dentry_lock(struct dentry *dentry)
{
    spin_lock(&dentry->d_lock);
}

static inline void dentry_unlock(struct dentry *dentry)
{
    spin_unlock(&dentry->d_lock);
}

static void dentry_unlink(struct dentry *dentry)
{
    if (dentry != NULL && dentry->d_parent != NULL && dentry->d_parent != dentry &&
        !list_empty(&dentry->d_child)) {
        list_del(&dentry->d_child);
    }
}

static hval_t dentry_cache_hash(const struct hlist_node *node)
{
#define FNV_PRIME 16777619u
#define FNV_OFFSET 2166136261u
    struct dentry *dentry = container_of(node, struct dentry, d_lru_cache_node);
    uintptr_t parent_ptr = (uintptr_t)dentry->d_parent;
    u64 h = FNV_OFFSET ^ parent_ptr;

    for (u32 i = 0; i < dentry->d_name.len; i++) {
        h ^= (unsigned char)dentry->d_name.name[i];
        h *= FNV_PRIME;
    }

    return h;
}

static int dentry_cache_compare(const struct hlist_node *node_a, const struct hlist_node *node_b)
{
    struct dentry *a = container_of(node_a, struct dentry, d_lru_cache_node);
    struct dentry *b = container_of(node_b, struct dentry, d_lru_cache_node);

    if (a->d_parent == b->d_parent && a->d_name.len == b->d_name.len &&
        memcmp(a->d_name.name, b->d_name.name, a->d_name.len) == 0) {
        return 0;
    }
    return 1;
}

static int dentry_cache_free(struct lru_node *node)
{
    struct dentry *dentry = container_of(node, struct dentry, d_lru_cache_node);

    if (dentry->d_inode != NULL) {
        iput(dentry->d_inode);
        dentry->d_inode = NULL;
    }
    dentry_unlink(dentry);
    kfree(dentry->d_name.name);
    kfree(dentry);
    return 0;
}

static struct lru_ops dentry_lru_ops = {
    .free = dentry_cache_free,
    .sync = NULL,
};

static struct hash_ops dentry_hash_ops = {
    .hash_func = dentry_cache_hash,
    .hash_compare = dentry_cache_compare,
};

int dcache_init(void)
{
    g_dcache = lru_cache_create(128, &dentry_lru_ops, &dentry_hash_ops);
    CHECK(g_dcache != NULL, "fs: create dcache failed", return -1;);
    return 0;
}

void dcache_destroy(void)
{
    if (g_dcache != NULL) {
        lru_cache_destroy(g_dcache);
        g_dcache = NULL;
    }
}

struct dentry *d_lookup(struct dentry *parent, const struct qstr *name) {
    struct dentry key;
    struct lru_node *found = NULL;

    CHECK(g_dcache != NULL, "fs: dcache is not initialized", return NULL;);
    CHECK(parent != NULL, "fs: invalid lookup parent", return NULL;);
    CHECK(name != NULL && name->name != NULL, "fs: invalid lookup name", return NULL;);

    memset(&key, 0, sizeof(key));
    key.d_parent = parent;
    key.d_name.name = name->name;
    key.d_name.len = name->len;
    lru_node_reset(&key.d_lru_cache_node);

    found = lru_cache_find(g_dcache, &key.d_lru_cache_node);
    if (found == NULL) {
        return NULL;
    }

    return container_of(found, struct dentry, d_lru_cache_node);
}

static struct dentry *d_alloc_n(struct dentry *parent, const char *name, u32 len) {
    struct dentry *dentry = NULL;

    CHECK(name != NULL, "fs: invalid dentry name", return NULL;);

    dentry = kmalloc(sizeof(*dentry));
    CHECK(dentry != NULL, "fs: alloc dentry failed", return NULL;);
    memset(dentry, 0, sizeof(*dentry));

    dentry->d_name.name = kmalloc(len + 1);
    CHECK(dentry->d_name.name != NULL, "fs: alloc qstr failed", kfree(dentry); return NULL;);
    memcpy(dentry->d_name.name, name, len);
    dentry->d_name.name[len] = '\0';
    dentry->d_name.len = len;

    dentry->d_parent = parent;
    dentry->d_sb = parent ? parent->d_sb : NULL;
    dentry->d_count = 1;
    spin_lock_init(&dentry->d_lock);
    INIT_LIST_HEAD(&dentry->d_child);
    INIT_LIST_HEAD(&dentry->d_subdirs);
    lru_node_reset(&dentry->d_lru_cache_node);

    if (parent != NULL) {
        list_add(&parent->d_subdirs, &dentry->d_child);
    }

    CHECK(g_dcache != NULL, "fs: dcache is not initialized",
          dentry_unlink(dentry);
          kfree(dentry->d_name.name);
          kfree(dentry);
          return NULL;);

    CHECK(lru_cache_add(g_dcache, &dentry->d_lru_cache_node) == 0, "fs: cache dentry failed",
          dentry_unlink(dentry);
          kfree(dentry->d_name.name);
          kfree(dentry);
          return NULL;);

    return dentry;
}

struct dentry *d_alloc(struct dentry *parent, const char *name) {
    CHECK(name != NULL, "fs: invalid dentry name", return NULL;);
    return d_alloc_n(parent, name, strlen(name));
}

struct dentry *d_alloc_qstr(struct dentry *parent, const struct qstr *name) {
    CHECK(name != NULL && name->name != NULL, "fs: invalid qstr name", return NULL;);
    return d_alloc_n(parent, name->name, name->len);
}

void d_add(struct dentry *dentry, struct inode *inode) {
    CHECK(dentry != NULL, "fs: invalid d_add dentry", return;);

    dentry_lock(dentry);
    if (dentry->d_inode != NULL) {
        iput(dentry->d_inode);
    }
    dentry->d_inode = inode ? igrab(inode) : NULL;
    dentry_unlock(dentry);
}



void d_destroy(struct dentry *dentry) {
    if (dentry == NULL || g_dcache == NULL) {
        return;
    }

    lru_cache_remove(g_dcache, &dentry->d_lru_cache_node);
}

struct dentry *d_make_root(struct inode *root_inode) {
    struct dentry *root = NULL;

    CHECK(root_inode != NULL, "fs: invalid root inode", return NULL;);

    root = d_alloc(NULL, "/");
    CHECK(root != NULL, "fs: alloc root dentry failed", return NULL;);

    d_add(root, root_inode);
    root->d_parent = root;
    root->d_sb = root_inode->i_sb;
    return root;
}

struct dentry *dget(struct dentry *dentry) {
    CHECK(dentry != NULL, "fs: invalid dget dentry", return NULL;);

    // dentry_lock(dentry);
    dentry->d_count++;
    // dentry_unlock(dentry);
    if (g_dcache != NULL) {
        lru_cache_touch(g_dcache, &dentry->d_lru_cache_node);
    }
    return dentry;
}

void dput(struct dentry *dentry) {
    int count = 0;

    if (dentry == NULL) {
        return;
    }

    dentry_lock(dentry);
    if (dentry->d_count > 0) {
        dentry->d_count--;
    }
    count = dentry->d_count;
    dentry_unlock(dentry);

    if (count == 0) {
        d_destroy(dentry);
    } else if (g_dcache != NULL) {
        lru_cache_touch(g_dcache, &dentry->d_lru_cache_node);
    }
}
