/**
 * @FilePath     : /ZZZ-OS/include/os/fb.h
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-17 15:30:10
 * @LastEditTime : 2026-07-17 15:48:15
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/
#ifndef _OS_FB_H
#define _OS_FB_H

#include <os/types.h>
#include <fs/cdev.h>
#include <os/device.h>

struct fb_info {
    u32 width;
    u32 height;
    u32 pitch;
    u32 bpp;
    size_t size;

    void *screen_base;
    phys_addr_t screen_phys;
    
    dev_t devt;
    struct cdev cdev;
    struct device *dev;
};

int register_framebuffer(struct fb_info *info, struct device *parent);
void unregister_framebuffer(struct fb_info *info);

#endif