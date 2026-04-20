#ifndef FS2_NAMEI_H
#define FS2_NAMEI_H

#include <fs/types.h>

int path_lookup(const char *path, struct path *out);
int path_parentat(const char *path, struct path *parent, struct qstr *last);

void path_get(const struct path *path);
void path_put(const struct path *path);

#endif
