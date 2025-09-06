/** 
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_dir.c
 * * @Description: * @Author: scuec_weiqiang scuec_weiqiang@qq.com 
 * @LastEditTime: 2025-09-04 18:41:09
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * * @Copyright : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025. */

#include "vfs_types.h"
#include "vfs_icache.h" 
#include "vfs_pcache.h"
#include "ext2_cache.h" 
#include "ext2_fs.h"
#include "ext2_block.h" 
#include "ext2_super.h" 
#include "ext2_inode.h"

#include "check.h"
#include "list.h"
#include "page_alloc.h" 
#include "string.h"

#define EXT2_ENTRY_LEN(name_len) (((sizeof(ext2_dir_entry_2_t) + (name_len) + 3) / 4) * 4) 

static uint32_t ext2_mode_to_entry_type(uint32_t i_mode)
{
    switch (EXT2_GET_TYPE(i_mode))
    {
        case EXT2_S_IFDIR:
            return EXT2_FT_DIR;
        case EXT2_S_IFREG:
            return EXT2_FT_REG_FILE;
        default:
            return EXT2_FT_UNKNOWN;
    }
}

static int64_t ext2_init_dot_entries(vfs_inode_t *dir, uint32_t parent_inode)
{
    CHECK(dir != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = dir->i_sb;
    vfs_page_t *page = vfs_pget(dir, 0);

    ext2_dir_entry_2_t *dot = (ext2_dir_entry_2_t *)page;
    dot->inode = dir->i_ino;
    dot->name_len = 1;
    dot->file_type = EXT2_FT_DIR;
    dot->rec_len = 12;
    memcpy(dot->name, ".", 1);

    ext2_dir_entry_2_t *dotdot = (ext2_dir_entry_2_t *)(page + dot->rec_len);
    dotdot->inode = parent_inode;
    dotdot->name_len = 2;
    dotdot->file_type = EXT2_FT_DIR;
    dotdot->rec_len = vfs_sb->s_block_size - dot->rec_len;
    memcpy(dotdot->name, "..", 2);

    page->dirty = true;
    vfs_pput(page);
    
    return 0;
} 

/** 
* @brief 从虚拟文件系统（VFS）中获取指定文件或目录的dentry信息 
* 该函数在指定的父目录（parent_dentry）下查找名为name的文件或目录，并将找到的dentry信息复制到dentry_ret中。 
* 如果在父目录的子目录链表中找到了同名文件或目录，则直接返回对应的inode索引。 
* 如果在子目录链表中未找到，则从磁盘上读取目录项，并在遍历所有相关的block后返回找到的inode索引。 
* @param vfs_sb VFS超级块指针，包含文件系统的全局信息 
* @param dir 父目录的dentry指针
* @param name 要查找的文件或目录名 
* @param dentry_ret 用于存储找到的dentry信息的指针 
* @return 如果成功找到文件或目录，则返回对应的inode索引；如果未找到，则返回-1。 
*/
vfs_inode_t *ext2_lookup(vfs_inode_t *dir, const char *name)
{
    CHECK(dir != NULL, "", return -1;);
    CHECK(name != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = dir->i_sb;

    uint64_t page_num = (dir->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    vfs_inode_t *inode_ret = NULL; 
    
    for (uint64_t i = 0; i < page_num; i++)
    {
        vfs_page_t *page = vfs_pget(dir, i);
        uint32_t offset = 0; 
        ext2_dir_entry_2_t *entry = (ext2_dir_entry_2_t *)page->data; 
        // 开始对这个block中的目录项进行遍历
        while (offset < VFS_PAGE_SIZE)
        {
            entry = (ext2_dir_entry_2_t *)(page->data + offset);
            if (entry->inode != 0 && strcmp(entry->name, name) == 0)
            {
                inode_ret = vfs_iget(vfs_sb, entry->inode);
                return inode_ret;
            }
            else
            {
                offset += entry->rec_len;
            }
        }
    }
    return -1;
}

int64_t ext2_init_new_inode(vfs_inode_t *inode, uint32_t i_mode)
{
    CHECK(inode != NULL && inode->i_private != NULL, "", return -1;);
    CHECK(inode->i_sb != NULL && inode->i_sb->s_private != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = inode->i_sb;
    ext2_inode_t *new_inode = (ext2_inode_t*)inode->i_private;

    switch(EXT2_GET_TYPE(i_mode))
    {
        case EXT2_S_IFDIR:
            int64_t new_block_idx_ret = ext2_alloc_bno(vfs_sb,ext2_select_block_group(vfs_sb)); // 分配新的块
            CHECK(new_block_idx_ret >= 0, "", return -1;);       // 检查分配块是否成功
            new_inode->i_block[0] = (uint64_t)new_block_idx_ret; // 更新块索引
            new_inode->i_blocks += vfs_sb->s_block_size / 512;
            new_inode->i_size = vfs_sb->s_block_size;
            new_inode->i_links_count = 2; // . 和 ..
            break;
        case EXT2_S_IFREG:
            new_inode->i_size = 0;
            new_inode->i_block[0] = 0;
            new_inode->i_blocks = 0;
        default:
            new_inode->i_block[0] = 0;
            new_inode->i_blocks = 0;
            break;
    }
    uint32_t current_time = get_current_unix_timestamp(UTC8);
    new_inode->i_atime = current_time;
    new_inode->i_ctime = current_time;
    new_inode->i_mtime = current_time;
    new_inode->i_dtime = 0;
    new_inode->i_mode = i_mode;
    new_inode->i_uid = 0; // root
    new_inode->i_gid = 0; // root

    // 同步公共字段
    inode->i_size = new_inode->i_size;  
    inode->i_mode = new_inode->i_mode;
    inode->i_uid = new_inode->i_uid;
    inode->i_gid = new_inode->i_gid;
    inode->i_nlink = new_inode->i_links_count;
    inode->i_atime.tv_sec = new_inode->i_atime;
    inode->i_ctime.tv_sec = new_inode->i_ctime;
    inode->i_mtime.tv_sec = new_inode->i_mtime;

    inode->dirty = true; // 标记为脏，需要写回

    return 0;
}

int64_t ext2_init_new_entry(ext2_dir_entry_2_t *new_entry, const char *name, uint32_t inode_idx, uint32_t i_mode) 
{
    new_entry->inode = inode_idx; // 设置新目录项指向的inode索引
    new_entry->name_len = strlen(name); // 设置新目录项的名称长度 
    new_entry->file_type = ext2_mode_to_entry_type(i_mode); // 设置entry文件类型
    memcpy(new_entry->name, name, new_entry->name_len); // 设置新目录项的名称 
}

void print_ext2_inode_member(vfs_superblock_t *vfs_sb, ext2_inode_t *inode)
{
    ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    printf("i_mode: %x\n", inode->i_mode);
    printf("i_uid: %d\n", inode->i_uid);
    printf("i_size: %du\n", inode->i_size);
    printf("i_atime: %du\n", inode->i_atime);
    printf("i_ctime: %du\n", inode->i_ctime);
    printf("i_mtime: %du\n", inode->i_mtime);
    printf("i_dtime: %du\n", inode->i_dtime);
    printf("i_gid: %d\n", inode->i_gid);
    printf("i_links_count: %d\n", inode->i_links_count);
    printf("i_blocks: %du\n", inode->i_blocks);
    printf("i_flags: %du\n", inode->i_flags);
    printf("i_block: ");
    for (int i = 0; i < 11; i++)
    {
        printf("%du ", inode->i_block[i]);
    }
    printf("\n");
    printf("i_generation: %du\n", inode->i_generation);
    printf("i_file_acl: %du\n", inode->i_file_acl);
    printf("i_dir_acl: %du\n", inode->i_dir_acl);
    printf("i_faddr: %du\n", inode->i_faddr);
    ext2_load_block_bitmap_cache(vfs_sb, 0);
    ext2_load_inode_bitmap_cache(vfs_sb, 0);
    bitmap_test_bit(fs_info->bbm_cache.bbm, inode->i_block[0]) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");
    bitmap_test_bit(fs_info->ibm_cache.ibm, 0xb) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");
}

// 辅助结构：查到的空槽信息
typedef struct {
    uint64_t page_index;      // 页号
    uint32_t offset;           // 在块内的偏移
    uint32_t prev_offset;      // 如果需要修改 prev_entry->rec_len
    uint32_t prev_real_len;   // prev_entry 的真实大小
    uint32_t free_len;        // 可用于新项的空间
    bool     found;            // 找到
    // bool     prev_is_empty_inode; // prev entry 的 inode==0
} dir_slot_t;
static int64_t ext2_find_free_slot_in_page(vfs_page_t *page, uint32_t need_len, dir_slot_t *out)
{
    uint8_t *page_buf = page->data;
    uint32_t offset = 0;
    ext2_dir_entry_2_t *entry = NULL;
    uint32_t entry_real_len = 0;

    while (offset < VFS_PAGE_SIZE)
    {
        entry = (ext2_dir_entry_2_t *)(page_buf + offset); // 获取当前目录项
        entry_real_len = EXT2_ENTRY_LEN(entry->name_len);// 计算当前目录项的真实大小,4字节对齐
        if (entry->inode == 0)
        {
            out->offset = offset;
            out->prev_offset = offset;
            out->prev_real_len = 0;
            out->free_len = entry->rec_len; // Fixed typo: private.out -> private->out
            out->found = true; // Fixed typo: private.out -> private->out
            out->page_index = page->index;
            // private->out->prev_is_empty_inode = true; // Fixed typo: private.out -> private->out
            return 0;
        }
        else if (entry->rec_len - entry_real_len >= need_len) // 如果空白位置足够大，可以在这里添加新目录项
        {
            out->offset = offset + entry_real_len; // 新目录项的位置
            out->prev_offset = offset;          // 需要修改的目录项位置
            out->free_len = entry->rec_len - entry_real_len; // 可用空间大小
            out->found = true;
            out->prev_real_len = entry_real_len;
            out->page_index = page->index;
            return 0;
        }
        offset += entry->rec_len; // 移动到下一个目录项
    }
    return -1;
}
int64_t ext2_get_slot(vfs_inode_t *dir, const char *name, dir_slot_t *slot_out)
{
    CHECK(dir != NULL, "", return -1;);
    CHECK(name != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = dir->i_sb;

    uint64_t page_num = (dir->i_size + VFS_PAGE_SIZE - 1) / VFS_PAGE_SIZE;
    for (uint64_t i = 0; i < page_num; i++)
    {
        vfs_page_t *page = vfs_pget(dir, i);
        uint32_t offset = 0; 
        uint32_t need_len = EXT2_ENTRY_LEN(strlen(name));
        ext2_find_free_slot_in_page(page,need_len,slot_out);
        if(slot_out->found == true)
        {
            vfs_pput(page);
            return 0;
        }
        vfs_pput(page);
    }
    return -1;
}

int64_t ext2_add_entry(vfs_inode_t *dir, dir_slot_t *slot, const char *name, uint32_t i_mode)
{
    CHECK(dir != NULL, "", return -1;);
    CHECK(name != NULL, "", return -1;);

    vfs_superblock_t *vfs_sb = dir->i_sb;

    vfs_inode_t *new_inode = vfs_new_inode(vfs_sb); // 创建新的inode 
    ext2_init_new_inode(new_inode, i_mode);// 初始化新的inode 
    ext2_init_dot_entries(new_inode, dir->i_ino); // 初始化 . 和 .. 目录项
    vfs_iput(new_inode); // 写回缓存

   
    vfs_page_t *page = vfs_pget(dir, slot->page_index); // 获取包含空槽的页
    ext2_dir_entry_2_t *new_entry = (ext2_dir_entry_2_t *)(page->data + slot->offset);
    ext2_dir_entry_2_t *prev_entry = (ext2_dir_entry_2_t *)(page->data + slot->prev_offset);
    if (slot->prev_real_len > 0) // 如果需要修改前一个目录项的rec_len
    {
        prev_entry->rec_len = slot->prev_real_len; // 调整前一个目录项的rec_len
    }
    new_entry->rec_len = slot->free_len - slot->prev_real_len; // 设置新目录项的rec_len
    ext2_init_new_entry(new_entry, name, new_inode->i_ino, i_mode); // 初始化新目录项
    
    page->dirty = true; // 标记页为脏
    vfs_pput(page); // 写回缓存

    print_ext2_inode_member(vfs_sb,new_inode->i_private); 

    ((ext2_inode_t*)dir->i_private)->i_links_count++; // 增加父目录链接计数 
    ((ext2_fs_info_t*)(vfs_sb->s_private))->group_desc[ext2_ino_group(vfs_sb,new_inode->i_ino)].bg_used_dirs_count++; 


    return new_inode->i_ino; 
} 

int64_t ext2_mkdir(vfs_inode_t *dir, const char* name, uint32_t i_mode) 
{ 
    CHECK(dir != NULL, "", return -1;);
    CHECK(name != NULL, "", return -1;);

    dir_slot_t slot = {0};
    ext2_get_slot(dir, name, &slot);
    ext2_add_entry(dir, &slot, name, i_mode);

    vfs_icache_sync(); // 同步inode缓存
    vfs_pcache_sync(); // 同步page缓存
    
    ext2_sync_cache(dir->i_sb); // 同步缓存 
    ext2_sync_super(dir->i_sb); // 同步superblock到磁盘 

    return 0; 
} 

vfs_inode_ops_t ext2_inode_ops = 
{ 
    .lookup = ext2_lookup, 
    .mkdir = ext2_mkdir, 
};



    // /**
    //  * @FilePath: /ZZZ/kernel/fs/ext2/ext2_dir.c
    //  * @Description:
    //  * @Author: scuec_weiqiang scuec_weiqiang@qq.com
    //  * @Date: 2025-08-12 18:21:37
    //  * @LastEditTime: 2025-08-31 19:59:47
    //  * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
    //  * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
    // */
    // #include "vfs_types.h"
    // #include "check.h"
    // #include "ext2_fs.h"
    // #include "list.h"
    // #include "string.h"
    // #include "page_alloc.h"
    // #include "vfs_inode.h"
    // #include "ext2_cache.h"
    // #include "ext2_block.h"
    // #include "ext2_super.h"

    // extern int64_t ext2_init_inode(vfs_superblock_t *vfs_sb, uint32_t i_mode, ext2_inode_t *new_inode);

    // static int64_t find_sub(block_adapter_t *adap,uint32_t block_index,uint32_t index)
    // {
    //     CHECK(adap != NULL,"",return -1;);

    //     uint32_t block_size = block_adapter_get_block_size(adap);
    //     uint32_t *buf= malloc(block_size);
    //     CHECK(buf!=NULL,"",return -1;);

    //     int64_t ret = block_adapter_read(adap,buf,block_index,1);
    //     CHECK(ret>=0,"",return -1;);

    //     ret = (int64_t)buf[index];
    //     free(buf);
    //     return ret;
    // }

    // /**
    // * @brief 处理EXT2文件系统中的间接块,根据索引号查找对应的实际物理块号。
    // *
    // * 根据索引号index查找并返回对应的物理块号。支持直接索引、一级间接索引、二级间接索引和三级间接索引。
    // *
    // * @param vfs_inode VFS索引节点指针
    // * @param index 索引号
    // *
    // * @return 成功时返回对应的块号，失败时返回-1
    // */
    // static int64_t ext2_handle_indirect_block(vfs_inode_t *vfs_inode,uint32_t index)
    // {
    //     CHECK(vfs_inode != NULL, "", return -1;);
    //     CHECK(vfs_inode->i_sb->s_private!=NULL,"",return-1;);

    //     uint32_t block_size = vfs_inode->i_sb->s_block_size;

    //     uint32_t per_block = block_size / sizeof(uint32_t);
    //     ext2_inode_t *inode = (ext2_inode_t*)vfs_inode->i_private;
    //     block_adapter_t *adap = (block_adapter_t*)vfs_inode->i_sb->adap;

    //     uint64_t first_index = 0;
    //     uint64_t second_index = 0;
    //     uint64_t third_index = 0;
    //     int64_t sub_block_index = 0;
    //     int64_t ret = -1;

    //     // 直接索引
    //     if (index < 12) {
    //         return inode->i_block[index];
    //     }

    //     // 一级间接索引
    //     index -= 12;
    //     if (index < per_block) {
    //         return  find_sub(adap,inode->i_block[12],index);
    //     }

    //     // 二级间接索引
    //     index -= per_block;
    //     if (index < per_block * per_block)
    //     {
    //         // 计算索引的过程类似于把数字245提取出百位数字2,十位数字4,个位数字5，道理是一样的
    //         first_index = index / per_block;
    //         sub_block_index = find_sub(adap,inode->i_block[13],first_index);

    //         second_index = index % per_block;
    //         ret = find_sub(adap,sub_block_index,second_index);
    //         return ret;
    //     }

    //     // 三级间接索引
    //     index -= per_block*per_block;
    //     if (index < per_block * per_block * per_block)
    //     {
    //         first_index = index / (per_block * per_block);
    //         sub_block_index = find_sub(adap,inode->i_block[14],first_index);

    //         second_index = index % (per_block * per_block) / per_block;
    //         sub_block_index = find_sub(adap,sub_block_index,second_index);

    //         third_index = index % per_block;
    //         ret = find_sub(adap,sub_block_index,third_index);
    //         return ret;
    //     }

    //     return -1;
    // }

    // #define EXT2_ENTRY_LEN(name_len) (((sizeof(ext2_dir_entry_2_t) + (name_len) + 3) / 4) * 4)

    // static uint8_t ext2_mode_to_filetype(uint32_t i_mode)
    // {
    //     switch (EXT2_GET_TYPE(i_mode)) {
    //         case EXT2_S_IFDIR: return EXT2_FT_DIR;
    //         case EXT2_S_IFREG: return EXT2_FT_REG_FILE;
    //         default: return EXT2_FT_UNKNOWN;
    //     }
    // }
    // typedef int (*iterate_cb_t)(struct iterate_ctx_t *ctx);

    // typedef struct iterate_ctx_t
    // {
    //     iterate_cb_t cb;
    //     void* private_data;
    //     struct iterate_ctx_t *sub_ctx;
    // }iterate_ctx_t;

    // // 辅助结构：查到的空槽信息
    // typedef struct {
    //     uint64_t block_index;      // 物理块号
    //     uint32_t offset;           // 在块内的偏移
    //     uint32_t prev_offset;      // 如果需要修改 prev_entry->rec_len
    //     uint32_t prev_true_size;   // prev_entry 的真实大小
    //     uint32_t free_size;        // 可用于新项的空间
    //     bool     found;            // 找到
    //     // bool     prev_is_empty_inode; // prev entry 的 inode==0
    // } dir_slot_t;

    // typedef struct find_free_slot_private_data
    // {
    //     uint8_t *page_buf;
    //     uint32_t block_size;
    //     uint32_t need_len;
    //     dir_slot_t *out;
    // } find_free_slot_private_data_t;
    // static int64_t ext2_find_free_slot_in_block(iterate_ctx_t *ctx)
    // {
    //     CHECK(ctx != NULL, "", return -1;);

    //     uint32_t offset = 0;
    //     ext2_dir_entry_2_t *entry = NULL;
    //     uint32_t entry_real_len = 0;

    //     find_free_slot_private_data_t *private = ctx->private_data;
    //     iterate_cb_t *cb = ctx->cb;

    //     while (offset < private->block_size )
    //     {
    //         entry = (ext2_dir_entry_2_t *)(private->page_buf + offset); // 获取当前目录项
    //         entry_real_len = EXT2_ENTRY_LEN(entry->name_len);// 计算当前目录项的真实大小,4字节对齐
    //         if (entry->inode == 0)
    //         {
    //             entry = entry;
    //             private->out->offset = offset;
    //             private->out->prev_offset = offset;
    //             private->out->prev_true_size = 0;
    //             private->out->free_size = entry->rec_len; // Fixed typo: private.out -> private->out
    //             private->out->found = true; // Fixed typo: private.out -> private->out
    //             // private->out->prev_is_empty_inode = true; // Fixed typo: private.out -> private->out
    //             return 0;
    //         }
    //         else if (entry->rec_len -entry_real_len >= private->need_len) // 如果空白位置足够大，可以在这里添加新目录项
    //         {
    //             private->out->offset = offset + entry_real_len; // 新目录项的位置
    //             private->out->prev_offset = offset;          // 需要修改的目录项位置
    //             private->out->free_size = entry->rec_len - entry_real_len; // 可用空间大小
    //             private->out->found = true;
    //             return 0;
    //         }
    //         offset += entry->rec_len; // 移动到下一个目录项
    //     }
    //     return -1;
    // }

    // typedef struct iterate_dir_blocks_private_data {
    //     vfs_inode_t *dir;
    //     iterate_ctx_t *sub_ctx;
    // } iterate_dir_blocks_private_data_t;
    // static int64_t ext2_iterate_dir_blocks(iterate_ctx_t *ctx)
    // {
    //     CHECK(ctx != NULL, "", return -1;);

    //     iterate_dir_blocks_private_data_t *private = ctx->private_data;
    //     vfs_inode_t *dir = private->dir;
    //     iterate_cb_t cb = ctx->cb;
    //     iterate_ctx_t *sub_ctx = ctx->sub_ctx;

    //     vfs_superblock_t *vfs_sb = private->dir->i_sb;
    //     uint32_t per_block = vfs_sb->s_block_size / sizeof(uint32_t);
    //     uint8_t *buf = malloc(vfs_sb->s_block_size);
    //     if (!buf) return -1;

    //     for (uint64_t i = 0; i < 12 + (uint64_t)per_block + (uint64_t)per_block*per_block; i++)
    //     {
    //         int64_t bno = ext2_handle_indirect_block(dir, i);
    //         if (bno <= 0) continue;
    //         if (block_adapter_read(vfs_sb->adap, buf, bno, 1) < 0) { free(buf); return -1;}

    //         int rc = cb(sub_ctx);

    //         if (rc != 0) { free(buf); return rc; } // cb 可返回非0提前停止

    //     }
    //     free(buf);
    //     return 0;
    // }

    // /**
    // * @brief 从虚拟文件系统（VFS）中获取指定文件或目录的dentry信息
    // *
    // * 该函数在指定的父目录（parent_dentry）下查找名为name的文件或目录，并将找到的dentry信息复制到dentry_ret中。
    // * 如果在父目录的子目录链表中找到了同名文件或目录，则直接返回对应的inode索引。
    // * 如果在子目录链表中未找到，则从磁盘上读取目录项，并在遍历所有相关的block后返回找到的inode索引。
    // *
    // * @param vfs_sb VFS超级块指针，包含文件系统的全局信息
    // * @param dir 父目录的dentry指针
    // * @param name 要查找的文件或目录名
    // * @param dentry_ret 用于存储找到的dentry信息的指针
    // *
    // * @return 如果成功找到文件或目录，则返回对应的inode索引；如果未找到，则返回-1。
    // */
    //  vfs_inode_t* ext2_lookup(vfs_inode_t *dir, const char *name)
    // {
    //     CHECK(dir != NULL, "", return -1;);
    //     CHECK(name != NULL, "", return -1;);

    //     vfs_superblock_t *vfs_sb = dir->i_sb;

    //     uint32_t per_block = vfs_sb->s_block_size / sizeof(uint32_t);
    //     uint64_t inode_idx = dir->i_ino;
    //     ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    //     uint8_t *buf = (uint8_t *)malloc(vfs_sb->s_block_size);
    //     vfs_inode_t *inode_ret = NULL;
    //     // 遍历该目录对应的inode下所有的block
    //     for (uint64_t i = 0; i < 12 + per_block * per_block + per_block*per_block*per_block; i++)
    //     {

    //         int64_t block_index = ext2_handle_indirect_block(dir,i); // 获取物理块号
    //         CHECK(block_index > 0, "", return -1;);
    //         block_adapter_read(vfs_sb->adap, buf, block_index, 1); // 读取物理块

    //         uint32_t offset = 0;
    //         ext2_dir_entry_2_t *entry = (ext2_dir_entry_2_t *)buf;
    //         // 开始对这个block中的目录项进行遍历
    //         while (offset < vfs_sb->s_block_size)
    //         {
    //             entry = (ext2_dir_entry_2_t *)(uint8_t *)(buf + offset);
    //             if (entry->inode != 0 && strcmp(entry->name,name) == 0)
    //             {
    //                 inode_ret = vfs_iget(vfs_sb, entry->inode);
    //                 free(buf);
    //                 return inode_ret;
    //             }
    //             else
    //             {
    //                 offset += entry->rec_len;
    //             }
    //         }
    //     }
    //     free(buf);
    //     return -1;
    // }

    // int64_t ext2_init_dot_entries(vfs_inode_t *dir, uint32_t parent_inode)
    // {
    //     CHECK(dir != NULL, "", return -1;);

    //     vfs_superblock_t *vfs_sb = dir->i_sb;

    //     uint8_t *buffer = malloc(vfs_sb->s_block_size);
    //     ext2_dir_entry_2_t *dot = (ext2_dir_entry_2_t *)buffer;
    //     dot->inode = dir->i_ino;
    //     dot->name_len = 1;
    //     dot->file_type = EXT2_FT_DIR;
    //     dot->rec_len = 12;
    //     memcpy(dot->name, ".", 1);

    //     ext2_dir_entry_2_t *dotdot = (ext2_dir_entry_2_t *)(buffer + dot->rec_len);
    //     dotdot->inode = parent_inode;
    //     dotdot->name_len = 2;
    //     dotdot->file_type = EXT2_FT_DIR;
    //     dotdot->rec_len = vfs_sb->s_block_size - dot->rec_len;
    //     memcpy(dotdot->name, "..", 2);

    //     uint32_t block_index = ext2_handle_indirect_block(dir,0);
    //     CHECK(block_index > 0, "", free(buffer); return -1;);
    //     int64_t ret = block_adapter_write(vfs_sb->adap, buffer, block_index, 1);
    //     CHECK(ret >= 0, "", free(buffer); return -1;);
    //     free(buffer);

    //     return 0;
    // }

    // // int64_t ext2_init_entry(vfs_superblock_t *vfs_sb, vfs_dentry_t *dir, const char *name, uint32_t new_inode_idx, uint32_t i_mode)
    // // {

    // // }
    // // void print_ext2_inode_member(vfs_superblock_t *vfs_sb, ext2_inode_t *inode)
    // // {
    // //     ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;
    // //     printf("i_mode: %x\n", inode->i_mode);
    // //     printf("i_uid: %d\n", inode->i_uid);
    // //     printf("i_size: %du\n", inode->i_size);
    // //     printf("i_atime: %du\n", inode->i_atime);
    // //     printf("i_ctime: %du\n", inode->i_ctime);
    // //     printf("i_mtime: %du\n", inode->i_mtime);
    // //     printf("i_dtime: %du\n", inode->i_dtime);
    // //     printf("i_gid: %d\n", inode->i_gid);
    // //     printf("i_links_count: %d\n", inode->i_links_count);
    // //     printf("i_blocks: %du\n", inode->i_blocks);
    // //     printf("i_flags: %du\n", inode->i_flags);
    // //     printf("i_block: ");
    // //     for (int i = 0; i < 11; i++) {
    // //         printf("%du ", inode->i_block[i]);
    // //     }
    // //     printf("\n");
    // //     printf("i_generation: %du\n", inode->i_generation);
    // //     printf("i_file_acl: %du\n", inode->i_file_acl);
    // //     printf("i_dir_acl: %du\n", inode->i_dir_acl);
    // //     printf("i_faddr: %du\n", inode->i_faddr);
    // //     ext2_load_block_bitmap_cache(vfs_sb,0);
    // //     ext2_load_inode_bitmap_cache(vfs_sb,0);
    // //     bitmap_test_bit(fs_info->bbm_cache.bbm,inode->i_block[0]) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");
    // //     bitmap_test_bit(fs_info->ibm_cache.ibm,0xb) ? printf("i_block[0] is set\n") : printf("i_block[0] is not set\n");

    // // }

    // int64_t ext2_add_entry(vfs_inode_t *dir, const char *name, uint32_t i_mode)
    // {
    //     CHECK(dir != NULL, "", return -1;);
    //     CHECK(name != NULL, "", return -1;);

    //     vfs_superblock_t *vfs_sb = dir->i_sb;

    //     uint64_t inode_idx = dir->i_ino; // 获得父目录的inode索引
    //     ext2_fs_info_t *fs_info = (ext2_fs_info_t *)vfs_sb->s_private;

    //     uint32_t per_block = vfs_sb->s_block_size / sizeof(uint32_t);
    //     uint8_t *buf = (uint8_t *)malloc(vfs_sb->s_block_size);
    //     vfs_inode_t *vfs_inode = NULL;

    //     ext2_dir_entry_2_t *prev_entry;
    //     ext2_dir_entry_2_t *new_entry;
    //     uint64_t new_entry_size = EXT2_ENTRY_LEN(strlen(name));
    //     uint64_t prev_entry_true_size = 0;
    //     uint64_t block_index = 0;

    //     vfs_inode = vfs_iget(vfs_sb, inode_idx);
    //     CHECK(vfs_inode != NULL, "",free(buf); return -1;);
    //     ext2_inode_t *ext2_inode = (ext2_inode_t *)vfs_inode->i_private;

    //     // 遍历该目录对应的inode下所有的block
    //     for (uint64_t i = 0; i < 12 + per_block * per_block + per_block*per_block*per_block; i++)
    //     {

    //         if( ext2_inode->i_block[i] == 0) // 前面的块都满了，要分配新的块来存储新的目录项
    //         {
    //             int64_t new_block_idx_ret = ext2_alloc_bno(vfs_sb,ext2_select_block_group(fs_info)); // 分配新的块
    //             CHECK(new_block_idx_ret >= 0, "", return -1;);       // 检查分配块是否成功
    //             ext2_inode->i_block[i] = (uint64_t)new_block_idx_ret; // 更新块索引
    //             // ext2_clear_block(vfs_sb, (uint64_t)new_block_idx_ret);   // 清空新分配的块
    //             ext2_inode->i_blocks += vfs_sb->s_block_size / 512;
    //         }

    //         block_index = ext2_handle_indirect_block(vfs_inode,i); // 获取物理块号
    //         CHECK(block_index > 0, "", free(buf); return -1;);
    //         block_adapter_read(vfs_sb->adap, buf, block_index, 1); // 读取物理块

    //         uint32_t offset = 0;
    //         ext2_dir_entry_2_t *entry = (ext2_dir_entry_2_t *)buf;
    //         // 开始对这个block中的目录项进行遍历
    //         while (offset < vfs_sb->s_block_size)
    //         {
    //             prev_entry = (ext2_dir_entry_2_t *)(buf + offset); // 获取当前目录项
    //             prev_entry_true_size = ((sizeof(ext2_dir_entry_2_t) + prev_entry->name_len + 3) / 4) * 4;// 计算当前目录项的真实大小,4字节对齐

    //             if (prev_entry->inode == 0)
    //             {
    //                 new_entry = prev_entry;
    //                 if (new_entry->rec_len == 0) // 如果当前块是新分配的
    //                 {
    //                     new_entry->rec_len = vfs_sb->s_block_size;
    //                 }
    //                 break;
    //             }
    //             else if (strcmp(prev_entry->name, name) == 0) // 如果目录项名称匹配
    //             {
    //                 // printf(YELLOW("Entry %s already exists in directory %d\n"),
    //                 //        name, dir_inode_idx);
    //                 free(buf); // 释放缓冲区
    //                 return -1;    // 返回错误，表示目录项已存在
    //             }
    //             else if (prev_entry->rec_len - prev_entry_true_size >= new_entry_size) // 如果空白位置足够大，可以在这里添加新目录项
    //             {
    //                 new_entry = (ext2_dir_entry_2_t *)((uint8_t *)prev_entry + prev_entry_true_size); // 新目录项的位置
    //                 new_entry->rec_len = prev_entry->rec_len - prev_entry_true_size; // 设置新目录项的长度
    //                 prev_entry->rec_len = prev_entry_true_size;     // 减少当前目录项的长度
    //                 goto new;
    //             }
    //             else
    //             {

    //             }
    //             offset += prev_entry->rec_len; // 移动到下一个目录项
    //         }
    //     }

    // new:
    //     vfs_inode_t *new_inode = vfs_create_inode(vfs_sb); // 创建新的inode
    //     vfs_sb->s_op->create_private_inode(new_inode); // 创建私有inode数据
    //     new_inode->i_ino = vfs_sb->s_op->alloc_ino(vfs_sb, ext2_select_inode_group(fs_info)); // 分配新的inode索引
    //     ext2_init_inode(vfs_sb, i_mode, new_inode->i_private); // 初始化新的inode
    //     ((ext2_inode_t*)dir->i_private)->i_links_count++; // 设置链接计数为1
    //     vfs_sb->s_op->write_inode(new_inode); // 写入新的inode到磁盘
    //     fs_info->group_desc[0].bg_used_dirs_count++; // 增加目录计数

    //     ext2_sync_cache(vfs_sb); // 同步缓存
    //     ext2_sync_super(vfs_sb); // 同步superblock到磁盘
    //     print_ext2_inode_member(vfs_sb,new_inode->i_private);

    //     new_entry->inode = new_inode->i_ino; // 设置新目录项的inode索引
    //     new_entry->name_len = strlen(name); // 设置新目录项的名称长度
    //     switch (EXT2_GET_TYPE(i_mode))  // 设置新目录项的类型
    //     {
    //         case EXT2_S_IFDIR:
    //             new_entry->file_type = EXT2_FT_DIR; // 设置文件类型
    //             vfs_dentry_t *new_dentry = (vfs_dentry_t *)malloc(sizeof(vfs_dentry_t));
    //             new_dentry->d_private = new_entry;
    //             ext2_init_dot_entries(new_inode,dir->i_ino);
    //             break;
    //         case EXT2_S_IFREG:
    //             new_entry->file_type = EXT2_FT_REG_FILE; // 设置文件类型
    //             break;
    //         default:
    //             new_entry->file_type = EXT2_FT_UNKNOWN; // 设置文件类型
    //             break;
    //     }
    //     memcpy(new_entry->name, name, new_entry->name_len); // 设置新目录项的名称

    //     block_adapter_write(vfs_sb->adap, buf, block_index, 1); // 写入新的目录项
    //     block_adapter_read(vfs_sb->adap, buf, ((ext2_inode_t*)new_inode->i_private)->i_block[0], 1); // 写入新的目录项
    //     free(buf);
    //     return new_inode->i_ino;
    // }

    // /**
    //  * @brief 获取路径的基本名称
    //  *
    //  * 该函数用于从给定的路径字符串中提取基本名称（即最后一个斜杠后的部分）。
    //  *
    //  * @param path 要处理的路径字符串
    //  *
    //  * @return 返回指向基本名称的指针，如果路径中没有斜杠，则返回整个路径字符串。
    //  */
    // static int64_t ext2_get_path_base_dir_name(const char *path,char *basename )
    // {
    //     CHECK(path != NULL, "", return -1;);
    //     CHECK(basename != NULL, "", return -1;);
    //     size_t len = strlen(path);
    //     if (len == 0)
    //     {
    //         basename[0] = '\0'; // 如果路径为空，返回空字符串
    //         return 0;
    //     }
    //     uint64_t basename_head = 0;
    //     uint64_t basename_tail = len;

    //     // 去掉尾部的 '/'（如果有）
    //     while (basename_tail > 1 && path[basename_tail - 1] == '/')
    //     {
    //         basename_tail--;
    //     }
    //     // 向前查找最后一个 '/'
    //     for (int64_t i = basename_tail - 1; i >= 0; i--)
    //     {
    //         if (path[i] == '/')
    //         {
    //             basename_head = i + 1;
    //             break;
    //         }
    //     }

    //     strncpy(basename, path + basename_head, basename_tail - basename_head);
    //     basename[basename_tail - basename_head] = '\0';

    //     return 0;
    // }

    // int64_t ext2_mkdir(struct vfs_inode *dir, struct vfs_dentry *dentry, uint32_t mode)
    // {
    //     return 0;
    // }

    // vfs_inode_ops_t ext2_inode_ops = {
    //     // .lookup = ext2_lookup,
    //     // .open = ext2_inode_open,
    //     // .close = ext2_inode_close,
    //     // .read = ext2_inode_read,
    //     // .write = ext2_inode_write,
    //     // .unlink = ext2_inode_unlink,
    //     // .readdir = ext2_inode_readdir,
    //     // .mkdir = ext2_mkdir,
    //     // .link = ext2_inode_link,
    // };