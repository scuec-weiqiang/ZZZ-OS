#include "ext2_fs.h"
#include "ext2_dir.h"
#include "vfs_types.h"
#include "check.h"

int64_t ext2_lookup(vfs_inode_t *dir,vfs_dentry_t *dentry)
{
    CHECK(dir != NULL,"",return -1;);
    CHECK(dentry != NULL,"",return -1;);
    vfs_inode_t *d_inode = ext2_find(dir,dentry->name.name);
    if(d_inode==NULL)
    {
        return -1;
    }

    dentry->d_inode = d_inode;
    return 0;
}

static int64_t ext2_add(vfs_inode_t *i_parent, vfs_dentry_t *dentry, uint32_t i_mode) 
{ 
    CHECK(i_parent != NULL, "", return -1;);

    vfs_inode_t *new_inode = vfs_inew(i_parent->i_sb); // 创建新的inode 
    ext2_init_new_inode(new_inode, i_mode);// 初始化新的inode 
    
    dentry->d_inode = new_inode; // 关联dentry和新inode
    vfs_iput(new_inode); // 写回缓存

    dir_slot_t slot = {0};
    ext2_get_slot(i_parent, dentry->name.len, &slot);
    ext2_add_entry(i_parent, dentry, &slot, i_mode);

    
    i_parent->dirty = true; // 标记父目录inode为脏
    i_parent->i_mtime.tv_sec = get_current_unix_timestamp(UTC8); // 更新修改时间
    
    
    if(EXT2_GET_TYPE(i_mode) == EXT2_FT_DIR)
    {
        ext2_init_dot_entries(new_inode, i_parent->i_ino); // 初始化 . 和 .. 目录项
        ((ext2_inode_t*)i_parent->i_private)->i_links_count++; // 增加父目录链接计数 
        ((ext2_fs_info_t*)(i_parent->i_sb->s_private))->group_desc[ext2_ino_group(i_parent->i_sb,dentry->d_inode->i_ino)].bg_used_dirs_count++;
    }

    vfs_icache_sync(); // 同步inode缓存
    vfs_pcache_sync(); // 同步page缓存
    
    ext2_sync_cache(i_parent->i_sb); // 同步缓存 
    ext2_sync_super(i_parent->i_sb); // 同步superblock到磁盘 

    return 0; 
} 


int64_t ext2_mkdir(vfs_inode_t *i_parent, vfs_dentry_t *dentry, uint32_t i_mode) 
{ 
    return ext2_add(i_parent, dentry, i_mode);
} 


int64_t ext2_rmdir(vfs_inode_t *i_parent, vfs_dentry_t *dentry)
{
    
}


vfs_inode_ops_t ext2_inode_ops = 
{ 
    .lookup = ext2_lookup, 
    .mkdir = ext2_mkdir, 
};