/**
 * @FilePath: /ZZZ-OS/fs/ext2/ext2_super.c
 * @Description:
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-12 18:21:56
 * @LastEditTime: 2025-10-29 22:26:24
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
 */
#include "fs/ext2/ext2_types.h"
#include "fs/vfs_types.h"
#include "os/malloc.h"
#include "fs/ext2/ext2_erros.h"
#include "os/check.h"
#include "fs/ext2/ext2_inode.h"
#include "fs/icache.h"
#include "os/string.h"


struct superblock* ext2_fill_super(struct fs_type *fs_type, struct block_device *bdev, int flags)
{
    CHECK(fs_type!=NULL && bdev!=NULL,"ext2_fill_super error!",goto clean;);

    int ret = 0;
    struct ext2_superblock *sb = malloc(EXT2_SUPERBLOCK_SIZE);
    CHECK(sb!=NULL,"ext2_fill_super error: malloc memory failed!",goto clean;);
    
    uint64_t read_cnt = (EXT2_SUPERBLOCK_SIZE + bdev->sector_size -1)/bdev->sector_size;
    uint64_t read_pos = EXT2_SUPERBLOCK_OFFSET / bdev->sector_size; 
    for(uint64_t i=0;i<read_cnt;i++)
    {
        ret =  bdev->read((uint8_t*)sb+bdev->sector_size*i,read_pos+i); 
        CHECK(ret>=0,"ext2_fill_super error: read disk failed!",goto clean;);
    }

    CHECK(sb->s_magic==EXT2_SUPER_MAGIC,"ext2_fill_super error: not ext2 filesystem!",goto clean;);
    
    // 获得文件系统块大小
    uint32_t block_size = 1024<<sb->s_log_block_size;

    // 注册该文件系统对应的块适配器
    ret = block_adapter_register(fs_type->name,bdev->name,block_size);
    CHECK(ret>=0,"ext2_fill_super error: block_adapter_register failed!",goto clean;);

    // 打开块适配器，后面可以按文件系统块为单位读写磁盘
    struct block_adapter *adap = block_adapter_open(fs_type->name);

    uint64_t group_cnt = (sb->s_blocks_count + sb->s_blocks_per_group -1)/sb->s_blocks_per_group;
    uint64_t gdt_block = (block_size==1024) ? 2:1 ;
    uint64_t gdb_count = (group_cnt*sizeof(struct ext2_groupdesc)+block_size-1)/block_size;
    struct ext2_groupdesc *gdt = malloc(group_cnt*sizeof(struct ext2_groupdesc));
    //读取块描述符
    ret = block_adapter_read(adap,gdt,gdt_block,gdb_count);
    CHECK(ret>=0,"ext2_fill_super error: read group desc failed!",goto clean;);

    /*准备文件系统信息*/
    struct ext2_fs_info *fs_info = malloc(sizeof(struct ext2_fs_info));
    CHECK(fs_info!=NULL,"ext2_fill_super error: alloc fsinfo memory error",goto clean;);
    fs_info->super = sb;
    fs_info->group_desc = gdt;
    fs_info->s_inodes_per_block = block_size/sb->s_inode_size;
    fs_info->s_blocks_per_group = sb->s_blocks_per_group;
    fs_info->s_inodes_per_group = sb->s_inodes_per_group;
    fs_info->s_desc_per_block = block_size/sizeof(struct ext2_groupdesc);
    fs_info->s_gdb_count = gdb_count;
    fs_info->s_groups_count = group_cnt;
    fs_info->s_inodes_count = sb->s_inodes_count;
    fs_info->s_blocks_count = sb->s_blocks_count;
    fs_info->s_free_blocks_count = sb->s_free_blocks_count;
    fs_info->s_free_inodes_count = sb->s_free_inodes_count;
    fs_info->s_first_data_block = sb->s_first_data_block;
    fs_info->disk_size = bdev->capacity;
    fs_info->s_ibmb_per_group = ((fs_info->s_inodes_per_group/8)+block_size-1)/block_size;
    fs_info->s_bbmb_per_group = ((fs_info->s_blocks_per_group/8)+block_size-1)/block_size;
    fs_info->s_itb_per_group = ((fs_info->s_inodes_per_group*sizeof(struct ext2_inode))+block_size-1)/block_size;
    fs_info->ibm_cache.cached_group = -1; // 初始化为-1，表示没有缓存
    fs_info->bbm_cache.cached_group = -1; // 初始化为-1，表示没有缓存
    fs_info->it_cache.cached_group = -1; // 初始化为-1，表示没有缓存
    fs_info->ibm_cache.dirty = false; // 初始化为false，表示位图未修改
    fs_info->bbm_cache.dirty = false; // 初始化为false，表示位图未修改
    fs_info->it_cache.dirty = false; // 初始化为false，表示位图未修改

    
    struct superblock *vfs_sb = malloc(sizeof(struct superblock));
    CHECK(vfs_sb!=NULL,"ext2_fill_super error!",goto clean;);
    vfs_sb->s_magic = sb->s_magic;
    vfs_sb->s_block_size =  block_size;
    vfs_sb->s_type = fs_type;
    vfs_sb->adap = adap; 
    vfs_sb->s_private = (void*)fs_info;
    extern struct super_ops ext2_s_ops;
    vfs_sb->s_ops = &ext2_s_ops;

    struct inode *root_inode = create_inode(vfs_sb);
    CHECK(root_inode!=NULL,"ext2_fill_super error!",goto clean;);

    root_inode->i_ino = EXT2_ROOT_INODE_IDX,
    root_inode->i_private = vfs_sb->s_ops->create_private_inode(root_inode);
    CHECK(root_inode->i_private != NULL,"ext2_fill_super error!",goto clean;);

    ret = vfs_sb->s_ops->read_inode(root_inode);
    CHECK(ret>=0,"ext2_fill_super error!",goto clean;);
    lru_insert(global_inode_cache,&root_inode->i_lru_cache_node);
    vfs_sb->s_root = root_inode;
    

    return vfs_sb;

clean:
    if(sb) free(sb);
    if(gdt) free(gdt);
    if(fs_info) free(fs_info);
    if(vfs_sb) free(vfs_sb);
    return NULL;
}

void ext2_kill_super(struct superblock *vfs_sb)
{
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)vfs_sb->s_private;
    free(fs_info);
    block_adapter_destory(vfs_sb->adap);
}

int ext2_sync_super(struct superblock *sb)
{
    CHECK(sb!=NULL,"ext2_sync_super error!",return -1;);
    CHECK(sb->s_private!=NULL,"ext2_sync_super error!",return -1;);
    
    struct ext2_fs_info *fs_info = (struct ext2_fs_info*)sb->s_private; 
    int ret;
    // 同步超级块
    uint64_t block_size = sb->s_block_size;
    uint64_t write_offset = EXT2_SUPERBLOCK_OFFSET % block_size; 
    uint64_t write_pos = EXT2_SUPERBLOCK_OFFSET / block_size; 
    uint64_t write_cnt = EXT2_SUPERBLOCK_SIZE / block_size; 
    uint8_t *super_buf = malloc(block_size);
    //先读
    ret = block_adapter_read(sb->adap,super_buf,write_pos,write_cnt);
    CHECK(ret>=0,"ext2_sync_super error: read super block failed!",return -1;);
    //再写
    memcpy(super_buf+write_offset,fs_info->super,EXT2_SUPERBLOCK_SIZE);
    ret = block_adapter_write(sb->adap,super_buf,write_pos,write_cnt);
    free(super_buf);
    CHECK(ret>=0,"ext2_sync_super error: write super block failed!",return -1;);


    // 同步块描述符
    uint64_t gdt_block = (block_size==1024) ? 2:1 ;
    ret = block_adapter_write(sb->adap,fs_info->group_desc,gdt_block,fs_info->s_gdb_count);
    CHECK(ret>=0,"ext2_sync_super error: write group desc failed!",return -1;);

    return 0;
}

struct fs_type ext2_fs_type = {
    .name="ext2",
    .mount=ext2_fill_super,
    .kill_sb=ext2_kill_super,

};