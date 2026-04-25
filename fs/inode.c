#include <fs/inode.h>
#include <fs/pagecache.h>
#include <os/check.h>
#include <os/container_of.h>
#include <os/kmalloc.h>
#include <os/lru.h>
#include <os/string.h>
#include <os/err.h>

static struct lru_cache *g_icache;

static inline void inode_lock(struct inode *inode) {
    spin_lock(&inode->i_lock);
}

static inline void inode_unlock(struct inode *inode) {
    spin_unlock(&inode->i_lock);
}

static hval_t inode_cache_hash(const struct hlist_node *node) {
    struct inode *inode = container_of(node, struct inode, d_lru_cache_node);
    uintptr_t sb_ptr = (uintptr_t)inode->i_sb;
    const uint32_t golden_ratio = 0x9E3779B9U;
    hval_t hash = (hval_t)sb_ptr * golden_ratio;
    hash ^= (hval_t)inode->i_ino * golden_ratio;
    hash ^= (hash >> 16);
    return hash;
}

static int inode_cache_compare(const struct hlist_node *a, const struct hlist_node *b)
{
    struct inode *inode_a = container_of(a, struct inode, d_lru_cache_node);
    struct inode *inode_b = container_of(b, struct inode, d_lru_cache_node);

    if (inode_a->i_sb == inode_b->i_sb && inode_a->i_ino == inode_b->i_ino) {
        return 0;
    }
    return 1;
}

static int inode_cache_sync(struct lru_node *node)
{
    struct inode *inode = container_of(node, struct inode, d_lru_cache_node);

    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->write_inode != NULL) {
        return inode->i_sb->s_op->write_inode(inode);
    }
    return 0;
}

static int inode_cache_free(struct lru_node *node)
{
    struct inode *inode = container_of(node, struct inode, d_lru_cache_node);

    inode_cache_sync(node);
    pagecache_invalidate_mapping(inode->i_mapping);
    if (inode->i_sb != NULL && inode->i_sb->s_op != NULL &&
        inode->i_sb->s_op->destroy_inode != NULL) {
        inode->i_sb->s_op->destroy_inode(inode);
        return 0;
    }
    kfree(inode);
    return 0;
}

static struct lru_ops inode_lru_ops = {
    .free = inode_cache_free,
    .sync = inode_cache_sync,
};

static struct hash_ops inode_hash_ops = {
    .hash_func = inode_cache_hash,
    .hash_compare = inode_cache_compare,
};

int icache_init(void) {
    g_icache = lru_cache_create(128, &inode_lru_ops, &inode_hash_ops);
    CHECK(g_icache != NULL, "fs: create icache failed", return -1;);
    return 0;
}

void icache_destroy(void) {
    if (g_icache != NULL) {
        lru_cache_destroy(g_icache);
        g_icache = NULL;
    }
}

struct inode *new_inode(struct super_block *sb) {
    struct inode *inode = NULL;

    CHECK(sb != NULL, "fs: invalid super for inode", return ERR_PTR(-EINVAL););
    if (sb->s_op != NULL && sb->s_op->alloc_inode != NULL) {
        inode = sb->s_op->alloc_inode(sb);
        if (IS_ERR(inode)) {
            return inode;
        }
    } else {
        inode = kmalloc(sizeof(*inode));
        CHECK(inode != NULL, "fs: alloc inode failed", return ERR_PTR(-ENOMEM););
        memset(inode, 0, sizeof(*inode));
    }

    inode->i_sb = sb;
    inode->i_mapping = &inode->i_data;
    inode->i_data.host = inode;
    inode->i_data.a_ops = NULL;
    spin_lock_init(&inode->i_data.lock);
    inode->i_data.nrpages = 0;
    inode->i_count = 1;
    spin_lock_init(&inode->i_lock);
    lru_node_reset(&inode->d_lru_cache_node);
    return inode;
}

struct inode *iget(struct super_block *sb, ino_t ino) {
    struct inode key;
    struct lru_node *found = NULL;
    struct inode *inode = NULL;

    CHECK(sb != NULL, "fs: invalid super for iget", return ERR_PTR(-EINVAL););
    CHECK(g_icache != NULL, "fs: icache is not initialized", return ERR_PTR(-EINVAL););

    memset(&key, 0, sizeof(key));
    key.i_sb = sb;
    key.i_ino = ino;
    lru_node_reset(&key.d_lru_cache_node);

    found = lru_cache_find(g_icache, &key.d_lru_cache_node);
    
    if (found != NULL) {
        inode = container_of(found, struct inode, d_lru_cache_node);
        inode->i_state = I_UPTODATE;
        return igrab(inode);
    }

    inode = new_inode(sb);

    CHECK(inode != NULL, "fs: new inode failed", return ERR_PTR(-ENOMEM););
    inode->i_ino = ino;
    inode->i_state = I_NEW;
    int ret = lru_cache_add(g_icache, &inode->d_lru_cache_node);

    if(ret != 0) {
          if (sb->s_op != NULL && sb->s_op->destroy_inode != NULL) {
              sb->s_op->destroy_inode(inode);
          } else {
              kfree(inode);
          }
          return ERR_PTR(ret);
    }

    return inode;
}

struct inode *igrab(struct inode *inode) {
    CHECK(inode != NULL, "fs: invalid igrab inode", return NULL;);

    inode_lock(inode);
    inode->i_count++;
    inode_unlock(inode);
    if (g_icache != NULL) {
        lru_cache_touch(g_icache, &inode->d_lru_cache_node);
    }
    return inode;
}

void iput(struct inode *inode) {
    int count = 0;

    if (inode == NULL) {
        return;
    }

    inode_lock(inode);
    if (inode->i_count > 0) {
        inode->i_count--;
    }
    count = inode->i_count;
    inode_unlock(inode);

    if (count == 0) {
        lru_cache_remove(g_icache, &inode->d_lru_cache_node);
    } else if (g_icache != NULL) {
        lru_cache_touch(g_icache, &inode->d_lru_cache_node);
    }
}
