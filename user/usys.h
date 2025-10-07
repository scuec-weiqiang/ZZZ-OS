/**
 * @FilePath: /ZZZ-OS/user/usys.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-02 17:42:59
 * @LastEditTime: 2025-10-04 22:12:04
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef _USYS_H
#define _USYS_H

extern void yield();
extern void print(const char *str,int len);
extern int open(const char *path, int flags);
extern int close(int fd);
extern int read(int fd, char *buf, int len);
extern int write(int fd, const char *buf, int len);
extern int creat(const char *path, int mode);
extern int mkdir(const char *path, int mode);

#endif