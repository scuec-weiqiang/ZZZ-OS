/**
 * @FilePath: /ZZZ-OS/fs/super.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:30
 * @LastEditTime: 2025-10-29 22:14:27
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
/**
 * @FilePath: /ZZZ/kernel/fs/vfs_super.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:30
 * @LastEditTime: 2025-08-21 13:24:55
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "fs/vfs_types.h"
#include "os/check.h"
#include "os/string.h"
#include "fs/ext2/ext2_types.h"
#include "os/malloc.h"
#include "os/printk.h"



/* 全局超级块链表 */
static LIST_HEAD(superblocks_lhead);

struct superblock *alloc_super(struct fs_type *fs_type)
{
    struct superblock *sb;
    
    // 1. 分配内存
    sb = malloc(sizeof(struct superblock));
    CHECK(sb != NULL, "vfs_alloc_super: Cannot allocate memory for superblock", return NULL;);
    
    // 2. 清零初始化
    memset(sb, 0, sizeof(struct superblock));
    
    // 3. 初始化链表
    INIT_LIST_HEAD(&sb->s_list);
    
    // 5. 设置默认值
    sb->s_block_size = 4096;        // 默认块大小
    
    // 6. 添加到全局超级块链表
    list_add(&sb->s_list, &superblocks_lhead);
    
    printk("VFS: Allocated new superblock %p for filesystem %s\n",
           sb, fs_type ? fs_type->name : "unknown");
    
    return sb;
}

void free_super(struct superblock *sb)
{
    CHECK(sb != NULL, "vfs_free_super: Invalid superblock", return;);
    
    // 从全局超级块链表中移除
    list_del(&sb->s_list);
    
    // sb->s_ops
    // 释放内存
    free(sb);

    printk("VFS: Freed superblock %p\n", sb);
}

int sync_super(struct superblock *sb)
{
    CHECK(sb != NULL, "vfs_sync_super: Invalid superblock", return -1;);
    CHECK(sb->s_ops != NULL && sb->s_ops->sync_fs != NULL, "vfs_sync_super: No sync operation defined", return -1;);

    return sb->s_ops->sync_fs(sb);
}

