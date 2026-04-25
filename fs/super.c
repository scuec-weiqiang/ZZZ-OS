#include <fs/super.h>
#include <os/check.h>
#include <os/kmalloc.h>
#include <os/list.h>
#include <os/string.h>

struct super_block *alloc_super(struct file_system_type *type) {
    struct super_block *sb = NULL;

    CHECK(type != NULL, "fs: invalid super type", return NULL;);

    sb = kmalloc(sizeof(struct super_block));
    CHECK(sb != NULL, "fs: alloc super failed", return NULL;);
    memset(sb, 0, sizeof(struct super_block));

    sb->s_type = type;
    spin_lock_init(&sb->s_lock);
    sb->s_active = 1;
    INIT_LIST_HEAD(&sb->s_instances);
    return sb;
}

void destroy_super(struct super_block *sb)
{
    if (sb == NULL) {
        return;
    }

    if (sb->s_op != NULL && sb->s_op->put_super != NULL) {
        sb->s_op->put_super(sb);
    }
    kfree(sb);
}
