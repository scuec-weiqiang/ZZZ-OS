/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_super.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-21 12:52:30
 * @LastEditTime: 2025-09-01 00:12:50
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
#include "vfs_types.h"
#include "check.h"
#include "string.h"
#include "ext2_fs.h"
#include "page_alloc.h"
#include "printf.h"



/* 全局超级块链表 */
static LIST_HEAD(vfs_superblocks_lhead);

vfs_superblock_t *vfs_alloc_super(vfs_fs_type_t *fs_type)
{
    vfs_superblock_t *sb;
    
    // 1. 分配内存
    sb = malloc(sizeof(vfs_superblock_t));
    CHECK(sb != NULL, "vfs_alloc_super: Cannot allocate memory for superblock", return NULL;);
    
    // 2. 清零初始化
    memset(sb, 0, sizeof(vfs_superblock_t));
    
    // 3. 初始化链表
    INIT_LIST_HEAD(&sb->s_list);
    
    // 5. 设置默认值
    sb->s_block_size = 4096;        // 默认块大小
    
    // 6. 添加到全局超级块链表
    list_add(&sb->s_list, &vfs_superblocks_lhead);
    
    printf("VFS: Allocated new superblock %p for filesystem %s\n",
           sb, fs_type ? fs_type->name : "unknown");
    
    return sb;
}

void vfs_free_super(vfs_superblock_t *sb)
{
    CHECK(sb != NULL, "vfs_free_super: Invalid superblock", return;);
    
    // 从全局超级块链表中移除
    list_del(&sb->s_list);
    
    // sb->s_ops
    // 释放内存
    free(sb);

    
    printf("VFS: Freed superblock %p\n", sb);
}

int64_t vfs_sync_super(vfs_superblock_t *sb)
{
    CHECK(sb != NULL, "vfs_sync_super: Invalid superblock", return -1;);
    CHECK(sb->s_ops != NULL && sb->s_ops->sync_fs != NULL, "vfs_sync_super: No sync operation defined", return -1;);

    return sb->s_ops->sync_fs(sb);
}

