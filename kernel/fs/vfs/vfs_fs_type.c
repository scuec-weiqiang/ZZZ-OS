/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_fs_type.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-31 20:47:10
 * @LastEditTime: 2025-09-01 22:44:22
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "list.h"
#include "check.h"

static list_t g_fs_types = {&g_fs_types, &g_fs_types};

int64_t vfs_register_fs(vfs_fs_type_t *fs_type)
{
    CHECK(fs_type != NULL, "Invalid fs_type", return -1;);
    CHECK(fs_type->name != NULL, "Invalid fs_type name", return -1;);
    CHECK(fs_type->mount != NULL, "Invalid fs_type mount function", return -1;);
    CHECK(fs_type->kill_sb != NULL, "Invalid fs_type kill_sb function", return -1;);

    list_add(&g_fs_types,&fs_type->fs_type_lnode);

    return 0;
}

int64_t vfs_unregister_fs(vfs_fs_type_t *fs_type)
{
    CHECK(fs_type != NULL, "Invalid fs_type", return -1;);
    CHECK(fs_type->name != NULL, "Invalid fs_type name", return -1;);
    CHECK(fs_type->mount != NULL, "Invalid fs_type mount function", return -1;);
    CHECK(fs_type->kill_sb != NULL, "Invalid fs_type kill_sb function", return -1;);

    list_del(&fs_type->fs_type_lnode);

    return 0;
}

vfs_fs_type_t* vfs_get_fs(const char* name)
{
    list_t *pos;
    list_for_each(pos,&g_fs_types)
    {
        vfs_fs_type_t *fs = container_of(pos,vfs_fs_type_t,fs_type_lnode);
        if(strcmp(fs->name,name) == 0)
        {
            return fs;
        }
    }
    return NULL;
}