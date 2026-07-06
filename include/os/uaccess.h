#ifndef __OS_UACCESS_H
#define __OS_UACCESS_H

#include <asm/uaccess.h>

extern int copy_from_user(char *dst, const char* src, size_t len);
extern int copy_to_user(char *dst, char* src, size_t len);
extern int copy_user_string(char *dst, size_t dst_len, uintptr_t user_ptr);
#endif /* __OS_UACCESS_H */