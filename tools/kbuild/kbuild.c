#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "objs.build"
#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 1024
#define OBJ_PREFIX "OBJ_"

static const char *selected_arch = "arm";

/*
将类似"/home/user/"的字符串组合为 "/home/user/.config" 或 "/home/user/lib/"
*/
int add_path(char *path, char *add) {
    if (!path || !add) {
        return -1;
    }
    // 删去回车符
    char *p = strstr(path, "\n");
    if (p) {
        *p = 0;
    }
    p = strstr(add, "\n");
    if (p) {
        *p = 0;
    }

    size_t len = strlen(path);
    p = &path[len];

    while(*p != '/')
    {
        p--;
    }
    p ++;

    len = strlen(add);
    memcpy(p,add,len);
    p += len;
    *p = 0;
    return 0;
}

/*
将类似 "/home/user/.config" 或 "/home/user/lib/"的字符串截断为 "/home/user/"
*/
int truncate_path(char *path) {
    if (!path) {
        return -1;
    }
    size_t len = strlen(path);
    char *p = &path[len];

    // 略过字符串结尾
    while (*p == '\n' || *p == '/' || *p == 0) {
        p--;
    }

    while (*p != '/') {
        p--;
    }
    p++;
    *p = 0;
    return 0;
}

static char *skip_spaces(char *p) {
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static char *match_obj_key(char *line) {
    char key[64];
    size_t i = 0;
    char *p = skip_spaces(line);

    if (strncmp(p, OBJ_PREFIX, strlen(OBJ_PREFIX)) != 0) {
        return NULL;
    }

    while (p[i] != '\0' && !isspace((unsigned char)p[i]) &&
           p[i] != '+' && p[i] != '=' && p[i] != ':') {
        if (i + 1 >= sizeof(key)) {
            return NULL;
        }
        key[i] = p[i];
        i++;
    }
    key[i] = '\0';

    if (strcmp(key, "OBJ_Y") == 0) {
        return p + i;
    }

    if (selected_arch != NULL) {
        char arch_key[64];

        snprintf(arch_key, sizeof(arch_key), "OBJ_%s", selected_arch);
        if (strcmp(key, arch_key) == 0) {
            return p + i;
        }
    }

    return NULL;
}

int parse(char* path) {
    if (!path) {
        printf("kbuild error: invalid path\n");
        goto clean;
    }

    add_path(path, FILE_NAME);
    FILE* f = fopen(path, "r");
    if (!f) {
        printf("kbuild error: open %s failed\n",path);
        goto clean;
    }
    truncate_path(path);

    char line[MAX_LINE_LEN];
    char *token;
    while (fgets(line,MAX_LINE_LEN,f)) {
        if (line[0] == '#' || line[0] == '\n'|| line[0] == '\r' || line[0] == 0) {
            continue;
        }
        char *p = match_obj_key(line);
        if (p) {
            while((*p == ' ') || (*p == '+') || (*p == '=') || (*p == ':')) {
                p++;
            }
            char *save_ptr;
            token = strtok_r(p, " ", &save_ptr);
            while (token) {
                if (strstr(token,".o")) {
                    add_path(path, token);
                    printf("OBJ_Y += %s\n",path);
                    truncate_path(path);
                } else if (strstr(token,"/")) {
                    add_path(path, token);
                    parse(path);
                    truncate_path(path);
                } else {
                }
                token = strtok_r(NULL, " ",&save_ptr);
            }
        }
    }

    fclose(f);
    return 0;

    clean:
    if (f) {
        fclose(f);
    }
    return -1;
}



int main(int argc, char *argv[]) {
     if (argc != 2) {
        fprintf(stderr, "用法：%s <初始目录>\n", argv[0]);
        return 1;
    }
    const char *arch = getenv("ARCH");
    if (arch != NULL && arch[0] != '\0') {
        selected_arch = arch;
    }
    char path[MAX_PATH_LEN];
    strcpy(path, argv[1]);
    parse(path); // 从指定初始目录（如 src/）开始解析
    return 0;
}
