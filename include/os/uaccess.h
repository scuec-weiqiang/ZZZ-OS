#ifndef __OS_UACCESS_H
#define __OS_UACCESS_H

#include <asm/uaccess.h>

extern int copy_from_user(char *dst, char* src, size_t len);
extern int copy_to_user(char *dst, char* src, size_t len);

#endif /* __OS_UACCESS_H */