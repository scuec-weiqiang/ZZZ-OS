/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_dir.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-07 20:27:27
 * @LastEditTime: 2025-09-07 20:27:58
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_DIR_H
#define EXT2_DIR_H

#include "vfs_types.h"
extern struct inode *ext2_find(struct inode *i_parent, const char *name);

// 辅助结构：查到的空槽信息
typedef struct dir_slot{
    u64 page_index;      // 页号
    u32 offset;           // 在块内的偏移
    u32 prev_offset;      // 如果需要修改 prev_entry->rec_len
    u32 prev_real_len;   // prev_entry 的真实大小
    u32 free_len;        // 可用于新项的空间
    bool     found;            // 找到
    // bool     prev_is_empty_inode; // prev entry 的 inode==0
} dir_slot_t;
extern int ext2_find_slot(struct inode *i_parent, size_t name_len, dir_slot_t *slot_out);

extern int ext2_init_dot_entries(struct inode *i_parent, u32 parent_inode);
extern int ext2_init_new_inode(struct inode *inode, u32 i_mode);
extern int ext2_init_new_entry(struct ext2_dir_entry_2 *new_entry, const char *name, u32 inode_idx, u32 i_mode);
extern int ext2_add_entry(struct inode *i_parent, struct dentry *dentry, dir_slot_t *slot,  u32 i_mode);
extern int ext2_remove_entry(struct inode *i_parent, struct dentry *dentry);

#endif