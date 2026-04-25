#include <fs/namei.h>
#include <fs/dcache.h>
#include <fs/namespace.h>
#include <os/check.h>
#include <os/types.h>
#include <os/check.h>
#include <os/string.h>
#include <os/kmalloc.h>
#include <fs/inode.h>
#include <os/sched.h>

struct path_cursor {
    const char *path;
    uint32_t pos;
};
/**
 * @brief 获取路径中的下一个组件
 *
 * 该函数用于从路径游标中提取下一个组件（文件或目录名）。
 *
 * @param cur 指向路径游标的指针
 * @param name 指向用于存储组件名称的qstr结构体的指针
 *
 * @return 返回0表示成功，返回-1表示失败，返回1表示没有更多组件
 */
int next_component(struct path_cursor *cur, struct qstr *name) {
    uint32_t start;
    uint32_t end;

    if (cur == NULL || name == NULL || cur->path == NULL) {
        return -1;
    }

    while (cur->path[cur->pos] == '/') {
        cur->pos++;
    }

    if (cur->path[cur->pos] == '\0') {
        return 1;
    }

    start = cur->pos;
    end = start;

    while (cur->path[end] != '\0' && cur->path[end] != '/') {
        end++;
    }

    name->name = (char *)&cur->path[start];
    name->len = end - start;
    cur->pos = end;
    return 0;
}

void path_get(const struct path *path) {
	mntget(path->mnt);
	dget(path->dentry);
}

void path_put(const struct path *path) {
	mntget(path->mnt);
	dput(path->dentry);
}

int path_lookup(const char *path, struct path *out) {
    CHECK(path != NULL, "fs: invalid path", return -1;);
    CHECK(out != NULL, "fs: invalid lookup output", return -1;);
    CHECK(path[0] == '/', "fs: only absolute paths are supported now", return -1;);

    if (path[1] == '\0') {
        out->mnt = current->fs->root.mnt;
        out->dentry = current->fs->root.dentry;
        return out->dentry ? 0 : -1;
    }

    struct path_cursor cur;
    struct dentry *parent_dir = current->fs->root.dentry;
    struct vfsmount *mnt = NULL;
    struct dentry *child_dir = NULL;
    struct dentry *cached = NULL;
    struct dentry *dentry;
    struct qstr name;

    cur.path = path;
    cur.pos = 1; // skip leading '/'

    while (1) {
        int ret = next_component(&cur, &name);
        if (ret < 0) {
           goto err_out;
        } else if (ret > 0) {
            break;
        }

        
        cached = d_lookup(parent_dir, &name);

        // 先尝试在 dcache 中查找，否则才从磁盘找
        if (cached != NULL) {
            child_dir = dget(cached);
        } else {
            
            dentry = d_alloc_qstr(parent_dir, &name);
            child_dir = parent_dir->d_inode->i_op->lookup(parent_dir->d_inode, dentry, 0);
        }

        if (child_dir == NULL) {
            goto err_out;
        }

        parent_dir = child_dir;
        if (vfs_search_mount(parent_dir, &mnt) == 0) {
            parent_dir = mnt->mnt_root;
        }
    }

    out->mnt = mnt;
    out->dentry = parent_dir;

    return 0;

err_out:
    out->mnt = NULL;
    out->dentry = NULL;
    return -1;
}

int path_parentat(const char *path, struct path *parent, struct qstr *last) {
    struct path_cursor cur;
    struct qstr name;
    struct dentry *curr;
    struct dentry *next;
    struct dentry *dentry;
    struct dentry *cached;
    struct vfsmount *mnt;

    if (path == NULL || parent == NULL || last == NULL)
        return -1;
    if (path[0] != '/')
        return -1;

    if (path[1] == '\0') {
        parent->mnt = current->fs->root.mnt;
        parent->dentry = current->fs->root.dentry;
        last->name = parent->dentry->d_name.name;
        last->len = parent->dentry->d_name.len;
        return parent->dentry ? 0 : -1;
    }

    cur.path = path;
    cur.pos = 1;

    curr = current->fs->root.dentry;
    mnt = current->fs->root.mnt;

    while (1) {
        int ret = next_component(&cur, &name);
        dprintk("next component: %d ,%s\n", name.len, name.name);
        if (ret != 0) {
            goto err_out;
        }
            
        // 先试探下一个 component
        struct path_cursor save = cur;
        struct qstr next_name;
        
        int next_ret = next_component(&save, &next_name);
        dprintk("try next component: %d\n", next_ret);
 
        // 如果后面没有更多 component
        // 当前 name 就是最后一个分量
        if (next_ret > 0) {
            parent->mnt = mnt;
            parent->dentry = curr;
            *last = name;
            return 0;
        }

        cached = d_lookup(curr, &name);
        if (cached != NULL) {
            next = dget(cached);
            dprintk("found cached dentry: %s\n", next->d_name.name);
        } else {
            // 否则 current/name 还是中间路径，要继续 lookup
            dprintk("lookup dentry:%d, %s\n", name.len, name.name);
            dentry = d_alloc_qstr(curr, &name);
            next = curr->d_inode->i_op->lookup(curr->d_inode, dentry, 0);
        }

        if (next == NULL)
            return -1;

        curr = next;
        if (vfs_search_mount(curr, &mnt) == 0) {
            dprintk("found mount at dentry: %s\n", curr->d_name.name);
            curr = mnt->mnt_root;
        }
    }

err_out:
    parent->mnt = NULL;
    parent->dentry = NULL;
    last->name = NULL;
    last->len = 0;
    return -1;
}

struct dentry* vfs_lookup(const char* path) {
    CHECK(path != NULL, "", return NULL;);

    struct path resolved;
    int ret = path_lookup(path, &resolved);
    CHECK(ret == 0, "vfs: path lookup failed", return NULL;);

    return resolved.dentry;
}

struct dentry* vfs_mkdir(const char* path,uint16_t mode) {
    CHECK(path != NULL, "", return NULL;);
    CHECK(mode > 0, "", return NULL;);

    struct path resolved_parent;
    struct qstr child_name;

    int ret = path_parentat(path, &resolved_parent, &child_name);

    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    struct dentry *child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    dprintk("get parent dentry %s, parent node = %xu\n", resolved_parent.dentry->d_name.name, resolved_parent.dentry->d_inode->i_private);

    ret = resolved_parent.dentry->d_inode->i_op->mkdir(resolved_parent.dentry->d_inode, child_dentry, mode);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }
    return child_dentry;
}

struct dentry *vfs_create(const char *path, uint16_t mode) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    CHECK(path != NULL, "", return NULL;);

    ret = path_parentat(path, &resolved_parent, &child_name);
    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    ret = resolved_parent.dentry->d_inode->i_op->create(resolved_parent.dentry->d_inode, child_dentry, mode);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
}

struct dentry *vfs_mknod(const char *path, uint16_t mode, dev_t dev) {
    struct path resolved_parent;
    struct qstr child_name;
    struct dentry *child_dentry = NULL;
    int ret = 0;

    CHECK(path != NULL, "", return NULL;);

    ret = path_parentat(path, &resolved_parent, &child_name);
    CHECK(ret == 0, "vfs: parent dir lookup failed", return NULL;);

    child_dentry = d_alloc_qstr(resolved_parent.dentry, &child_name);
    CHECK(child_dentry != NULL, "vfs: d_alloc failed for child dentry", return NULL;);

    ret = resolved_parent.dentry->d_inode->i_op->mknod(resolved_parent.dentry->d_inode, child_dentry, mode, dev);
    if (ret < 0) {
        dput(child_dentry);
        return NULL;
    }

    return child_dentry;
}
