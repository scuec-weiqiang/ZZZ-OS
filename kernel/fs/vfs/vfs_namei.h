#ifndef VFS_NAMEI_H
#define VFS_NAMEI_H

#include "types.h"

extern char *path_split(char *str, const char *delim);
extern int64_t base_dir_split(const char *path,char *dirname, char *basename);

#endif // VFS_NAMEI_H