/**
 * @FilePath: /ZZZ/kernel/fs/vfs/fs_init.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-23 19:20:48
 * @LastEditTime: 2025-09-07 20:14:18
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "vfs_types.h"
#include "fs_type.h"
#include "icache.h"
#include "pcache.h"
#include "dcache.h"
#include "mount.h"

#include "ext2_super.h"

int fs_init()
{
    fs_register(&ext2_fs_type);
    icache_init();
    pcache_init();
    dcache_init();
    mount_rootfs();
    return 0;
}