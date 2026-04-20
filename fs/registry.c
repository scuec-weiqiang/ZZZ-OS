#include <fs/fs.h>
#include <os/check.h>
#include <os/container_of.h>
#include <os/list.h>
#include <os/string.h>

static LIST_HEAD(g_filesystems);

int register_filesystem(struct file_system_type *fs)
{
    CHECK(fs != NULL, "fs: invalid filesystem", return -1;);
    CHECK(fs->name != NULL, "fs: invalid filesystem name", return -1;);

    INIT_LIST_HEAD(&fs->fs_list);
    list_add(&g_filesystems, &fs->fs_list);
    return 0;
}

struct file_system_type *get_fs_type(const char *name)
{
    struct list_head *pos = NULL;

    CHECK(name != NULL, "fs: invalid filesystem name", return NULL;);

    list_for_each(pos, &g_filesystems) {
        struct file_system_type *fs = container_of(pos, struct file_system_type, fs_list);
        if (strcmp(fs->name, name) == 0) {
            return fs;
        }
    }

    return NULL;
}
