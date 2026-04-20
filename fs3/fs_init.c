/**
 * @FilePath: /ZZZ-OS/fs/fs_init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-23 19:20:48
 * @LastEditTime: 2025-10-29 22:26:44
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <fs/vfs_types.h>
#include <fs/fs_type.h>
#include <fs/icache.h>
#include <fs/pcache.h>
#include <fs/dcache.h>
#include <fs/mount.h>

#include <fs/ext2/ext2_super.h>

int fs_init()
{
    fs_register(&ext2_fs_type);
    icache_init();
    pcache_init();
    dcache_init();
    mount_rootfs();
    return 0;
}