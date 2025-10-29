/**
 * @FilePath: /ZZZ-OS/fs/chrdev.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-10-09 21:10:57
 * @LastEditTime: 2025-10-09 23:30:31
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "fs/vfs_types.h"
#include "os/string.h"
#include "fs/chrdev.h"
#include "os/malloc.h"

#define MAX_CHRDEV 256
static struct chrdev chrdevs[MAX_CHRDEV] = {0};

int register_chrdev(dev_t major, const char *name, const struct file_ops *fops)
{
    for (int i = 0; i < MAX_CHRDEV; i++) 
    {
        if (chrdevs[i].fops == NULL) {
            chrdevs[i].major = major;
            chrdevs[i].name = strdup(name);
            chrdevs[i].fops = (struct file_ops*)fops;
            return 0;
        }
    }
    return -1;
}

int unregister_chrdev(dev_t major, const char *name) {
    for (int i = 0; i < MAX_CHRDEV; i++) {
        if (chrdevs[i].major == major || (strcmp(chrdevs[i].name, name) == 0) ) {
            chrdevs[i].major = 0;
            free((void*)chrdevs[i].name);
            chrdevs[i].fops = NULL;
            return 0;
        }
    }
    return -1;
}

struct file_ops *get_chr_fops(dev_t major)
{
    for (int i = 0; i < MAX_CHRDEV; i++) 
    {
        if (chrdevs[i].major == major)
            return chrdevs[i].fops;
    }
    return NULL;
}