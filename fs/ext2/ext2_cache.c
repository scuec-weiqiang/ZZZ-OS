#include "fs/vfs_types.h"
#include "fs/ext2/ext2_types.h"
#include "os/malloc.h"
#include "os/check.h"
#include "os/string.h"
#include "fs/block_adapter.h"

int ext2_sync_inode_bitmap_cache(struct superblock *vfs_sb)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->adap!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);

    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    uint32_t logic_block = fs_info->group_desc[fs_info->ibm_cache.cached_group].bg_inode_bitmap; 
    int ret = block_adapter_write(vfs_sb->adap,BITMAP_ARR(fs_info->ibm_cache.ibm),logic_block,fs_info->s_ibmb_per_group);
    CHECK(ret>=0,"",return -1;);
    return 0;
}

int ext2_load_inode_bitmap_cache(struct superblock *vfs_sb,uint64_t group)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->adap!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    CHECK(fs_info->s_groups_count>=group,"",return -1;);

    int ret = 0;
    //如果想读取的组已经是缓存中的组
    if(group == fs_info->ibm_cache.cached_group)
    {
        return 0; 
    }

    // 检查是否需要写入脏位图    
    if(fs_info->ibm_cache.dirty == true)
    {
        ret = ext2_sync_inode_bitmap_cache(vfs_sb);
        CHECK(ret>=0,"",return -1;);
    }

    struct bitmap *bm = bitmap_create(fs_info->s_ibmb_per_group*vfs_sb->s_block_size);
    CHECK(bm!=NULL,"",return -1;);
    ret = block_adapter_read(vfs_sb->adap,BITMAP_ARR(bm),fs_info->group_desc[group].bg_inode_bitmap,fs_info->s_ibmb_per_group);
    CHECK(ret>=0,"",free(bm);return -1;);
    fs_info->ibm_cache.dirty = false;
    fs_info->ibm_cache.ibm = bm;
    fs_info->ibm_cache.cached_group = group;
    return 0;
}

int ext2_sync_inode_table_cache(struct superblock *vfs_sb)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->adap!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);

    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    CHECK(fs_info->it_cache.it!=NULL,"",return -1;);
    uint32_t logic_block_start = fs_info->group_desc[fs_info->it_cache.cached_group].bg_inode_table; 
    int ret = block_adapter_write(vfs_sb->adap,fs_info->it_cache.it,logic_block_start,fs_info->s_itb_per_group);
    CHECK(ret>=0,"",return -1;);
    return 0;
}

int ext2_load_inode_table_cache(struct superblock *vfs_sb,uint64_t group)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->adap!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    CHECK(fs_info->s_groups_count>=group,"",return -1;);

    int ret = 0;
    //如果想读取的组已经是缓存中的组
    if(group == fs_info->it_cache.cached_group)
    {
        return 0; 
    }

    // 检查是否需要写入脏位图    
    if(fs_info->it_cache.dirty == true)
    {
        ret = ext2_sync_inode_table_cache(vfs_sb);
        CHECK(ret>=0,"",return -1;);
    }

    struct ext2_inode* it = malloc(fs_info->s_itb_per_group*vfs_sb->s_block_size);
    CHECK(it!=NULL,"",return -1;);
    ret = block_adapter_read(vfs_sb->adap,it,fs_info->group_desc[group].bg_inode_table,fs_info->s_itb_per_group);
    CHECK(ret>=0,"",free(it);return -1;);
    fs_info->it_cache.dirty = false;
    fs_info->it_cache.it = it;
    fs_info->it_cache.cached_group = group;
    return 0;
}

int ext2_select_inode_group(struct superblock *vfs_sb)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 

    if(fs_info->ibm_cache.ibm!=NULL) // 如果已经有缓存，就直接用这个组
    {
        return fs_info->ibm_cache.cached_group;
    }
    else
    {
        uint64_t max = 0;
        int ret = -1;
        for(uint64_t group=0;group<fs_info->s_groups_count;group++)
        {
            // 找到空闲inode最多的组
            if(fs_info->group_desc[group].bg_free_inodes_count>max)
            {
                // 如果没进if那么ret值不会被更新，返回-1
                max = fs_info->group_desc[group].bg_free_inodes_count;
                ret = (int)group;
            }
        }
        return ret;
    }

}

int ext2_sync_block_bitmap_cache(struct superblock *vfs_sb)
{
    CHECK(vfs_sb != NULL,"",return -1;);
    CHECK(vfs_sb->adap != NULL,"",return -1;);
    CHECK(vfs_sb->s_private != NULL,"",return -1;);

    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    uint32_t logic_block = fs_info->group_desc[fs_info->bbm_cache.cached_group].bg_block_bitmap; 
    int ret = block_adapter_write(vfs_sb->adap,BITMAP_ARR(fs_info->bbm_cache.bbm),logic_block,fs_info->s_bbmb_per_group);
    CHECK(ret>=0,"",return -1;);
    return 0;
}

int ext2_load_block_bitmap_cache(struct superblock *vfs_sb,int group)
{
    CHECK(vfs_sb != NULL,"",return -1;);
    CHECK(vfs_sb->adap != NULL,"",return -1;);
    CHECK(vfs_sb->s_private != NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    CHECK(fs_info->s_groups_count>=group,"",return -1;);

    int ret = 0;
    //如果想读取的组已经是缓存中的组
    if(group == fs_info->bbm_cache.cached_group)
    {
        return 0; 
    }

    // 检查是否需要写入脏位图    
    if(fs_info->bbm_cache.dirty)
    {
        ret = ext2_sync_block_bitmap_cache(vfs_sb);
        CHECK(ret>=0,"",return -1;);
    }

    struct bitmap *bm = bitmap_create(fs_info->s_bbmb_per_group*vfs_sb->s_block_size);
    CHECK(bm!=NULL,"",return -1;);
    ret = block_adapter_read(vfs_sb->adap,BITMAP_ARR(bm),fs_info->group_desc[group].bg_block_bitmap,fs_info->s_bbmb_per_group);
    CHECK(ret>=0,"",free(bm);return -1;);
    fs_info->bbm_cache.dirty = false;
    fs_info->bbm_cache.bbm = bm;
    fs_info->bbm_cache.cached_group = group;
    return 0;
}

int ext2_select_block_group(struct superblock *vfs_sb)
{
    CHECK(vfs_sb!=NULL,"",return -1;);
    CHECK(vfs_sb->s_private!=NULL,"",return -1;);
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private; 
    CHECK(fs_info->s_groups_count>0,"",return -1;);

    if(fs_info->bbm_cache.bbm!=NULL)
    {
        return fs_info->bbm_cache.cached_group;
    }
    else
    {
        int max = 0;
        int ret = -1;
        for(int group=0;group<fs_info->s_groups_count;group++)
        {
            if(fs_info->group_desc[group].bg_free_blocks_count>max)
            {
                // 如果没进if那么ret值不会被更新，返回-1
                max = fs_info->group_desc[group].bg_free_blocks_count;
                ret = (int)group;
            }
        }
        return ret;
    }
}

int ext2_sync_cache(struct superblock *sb)
{
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)sb->s_private;
    if(fs_info->ibm_cache.dirty)
    {
        int ret = ext2_sync_inode_bitmap_cache(sb);
        CHECK(ret>=0,"ext2_sync_super error: sync inode bitmap failed!",return -1;);
        fs_info->ibm_cache.dirty = false;
    }
    if(fs_info->bbm_cache.dirty)
    {
        int ret = ext2_sync_block_bitmap_cache(sb);
        CHECK(ret>=0,"ext2_sync_super error: sync block bitmap failed!",return -1;);
        fs_info->bbm_cache.dirty = false;
    }
    if(fs_info->it_cache.dirty)
    {
        int ret = ext2_sync_inode_table_cache(sb);
        CHECK(ret>=0,"ext2_sync_super error: sync inode table failed!",return -1;);
        fs_info->it_cache.dirty = false;
    }

    return 0;
}
