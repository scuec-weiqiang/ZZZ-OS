#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "objs.build"
#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 1024
#define OBJ "OBJ_Y"

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
        char *p = strstr(line,OBJ);
        if (p) {
            p += strlen(OBJ);

            while((*p == ' ') || (*p == '+') || (*p == '=') || (*p == ':')) {
                p++;
            }

            token = strtok(p, " ");
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
                token = strtok(NULL, " ");
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
    char path[MAX_PATH_LEN];
    strcpy(path, argv[1]);
    parse(path); // 从指定初始目录（如 src/）开始解析
    return 0;
}