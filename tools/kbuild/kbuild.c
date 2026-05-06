#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "objs.build"
#define MAX_PATH_LEN 1024
#define MAX_LINE_LEN 1024
#define OBJ_PREFIX "OBJ_"

#define MAX_CONFIGS 1024
#define MAX_CONFIG_NAME 128
#define MAX_CONFIG_VALUE 128

struct config {
    char name[MAX_CONFIG_NAME];
    char value[MAX_CONFIG_VALUE];
};

static struct config configs[MAX_CONFIGS];
static int nr_configs;

static const char *selected_arch = "arm";

static char *trim_left(char *s)
{
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

static void trim_right(char *s)
{
    size_t len = strlen(s);

    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static void trim(char *s)
{
    char *p = trim_left(s);

    if (p != s) {
        memmove(s, p, strlen(p) + 1);
    }

    trim_right(s);
}

static void config_set(const char *name, const char *value)
{
    for (int i = 0; i < nr_configs; i++) {
        if (strcmp(configs[i].name, name) == 0) {
            strncpy(configs[i].value, value, MAX_CONFIG_VALUE - 1);
            configs[i].value[MAX_CONFIG_VALUE - 1] = '\0';
            return;
        }
    }

    if (nr_configs >= MAX_CONFIGS) {
        fprintf(stderr, "kbuild error: too many configs\n");
        exit(1);
    }

    strncpy(configs[nr_configs].name, name, MAX_CONFIG_NAME - 1);
    configs[nr_configs].name[MAX_CONFIG_NAME - 1] = '\0';

    strncpy(configs[nr_configs].value, value, MAX_CONFIG_VALUE - 1);
    configs[nr_configs].value[MAX_CONFIG_VALUE - 1] = '\0';

    nr_configs++;
}

static const char *config_get(const char *name)
{
    for (int i = 0; i < nr_configs; i++) {
        if (strcmp(configs[i].name, name) == 0) {
            return configs[i].value;
        }
    }

    return "n";
}

static int config_enabled(const char *name)
{
    return strcmp(config_get(name), "y") == 0;
}

static int load_config(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[MAX_LINE_LEN];

    if (!f) {
        fprintf(stderr, "kbuild error: open config %s failed\n", path);
        return -1;
    }

    while (fgets(line, sizeof(line), f)) {
        trim(line);

        if (line[0] == '\0') {
            continue;
        }

        /*
         * 支持:
         * # CONFIG_FOO is not set
         */
        if (line[0] == '#') {
            char name[MAX_CONFIG_NAME];

            if (sscanf(line, "# %127s is not set", name) == 1) {
                config_set(name, "n");
            }

            continue;
        }

        /*
         * 支持:
         * CONFIG_FOO=y
         * CONFIG_BAR=n
         */
        char *eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';

            char *name = line;
            char *value = eq + 1;

            trim(name);
            trim(value);

            if (name[0] != '\0') {
                config_set(name, value);
            }
        }
    }

    fclose(f);
    return 0;
}

/*
 * 将类似 "/home/user/" 的字符串组合为 "/home/user/.config" 或 "/home/user/lib/"
 */
int add_path(char *path, const char *add)
{
    if (!path || !add) {
        return -1;
    }

    path[strcspn(path, "\r\n")] = '\0';

    char clean_add[MAX_PATH_LEN];
    strncpy(clean_add, add, sizeof(clean_add) - 1);
    clean_add[sizeof(clean_add) - 1] = '\0';
    clean_add[strcspn(clean_add, "\r\n")] = '\0';

    size_t len = strlen(path);
    if (len == 0) {
        return -1;
    }

    char *p = path + len;

    while (p > path && *p != '/') {
        p--;
    }

    if (*p == '/') {
        p++;
    }

    size_t base_len = p - path;
    size_t add_len = strlen(clean_add);

    if (base_len + add_len + 1 >= MAX_PATH_LEN) {
        fprintf(stderr, "kbuild error: path too long\n");
        return -1;
    }

    memcpy(p, clean_add, add_len);
    p += add_len;
    *p = '\0';

    return 0;
}

/*
 * 将类似 "/home/user/.config" 或 "/home/user/lib/" 的字符串截断为 "/home/user/"
 */
int truncate_path(char *path)
{
    if (!path) {
        return -1;
    }

    size_t len = strlen(path);
    if (len == 0) {
        return -1;
    }

    char *p = path + len - 1;

    while (p > path && (*p == '\n' || *p == '\r' || *p == '/' || *p == '\0')) {
        p--;
    }

    while (p > path && *p != '/') {
        p--;
    }

    if (*p == '/') {
        p++;
        *p = '\0';
        return 0;
    }

    return -1;
}

static char *skip_spaces(char *p)
{
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    return p;
}

static int obj_key_enabled(const char *key)
{
    if (strcmp(key, "OBJ_Y") == 0) {
        return 1;
    }

    if (selected_arch != NULL) {
        char arch_key[64];

        snprintf(arch_key, sizeof(arch_key), "OBJ_%s", selected_arch);
        if (strcmp(key, arch_key) == 0) {
            return 1;
        }
    }

    /*
     * 支持:
     * OBJ_$(CONFIG_OF)
     */
    if (strncmp(key, "OBJ_$(", 6) == 0) {
        const char *start = key + 6;
        const char *end = strchr(start, ')');

        if (end && end > start) {
            char config_name[MAX_CONFIG_NAME];
            size_t len = end - start;

            if (len >= sizeof(config_name)) {
                return 0;
            }

            memcpy(config_name, start, len);
            config_name[len] = '\0';

            return config_enabled(config_name);
        }
    }

    return 0;
}

static char *match_obj_key(char *line)
{
    char key[128];
    size_t i = 0;
    char *p = skip_spaces(line);

    if (strncmp(p, OBJ_PREFIX, strlen(OBJ_PREFIX)) != 0) {
        return NULL;
    }

    while (p[i] != '\0' &&
           !isspace((unsigned char)p[i]) &&
           p[i] != '+' &&
           p[i] != '=' &&
           p[i] != ':') {
        if (i + 1 >= sizeof(key)) {
            return NULL;
        }

        key[i] = p[i];
        i++;
    }

    key[i] = '\0';

    if (obj_key_enabled(key)) {
        return p + i;
    }

    return NULL;
}

static void strip_comment(char *line)
{
    char *p = strchr(line, '#');

    if (p) {
        *p = '\0';
    }
}

int parse(char *path)
{
    if (!path) {
        fprintf(stderr, "kbuild error: invalid path\n");
        return -1;
    }

    if (add_path(path, FILE_NAME) < 0) {
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "kbuild error: open %s failed\n", path);
        return -1;
    }

    truncate_path(path);

    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), f)) {
        strip_comment(line);
        trim(line);

        if (line[0] == '\0') {
            continue;
        }

        char *p = match_obj_key(line);
        if (!p) {
            continue;
        }

        while (*p == ' ' || *p == '\t' || *p == '+' || *p == '=' || *p == ':') {
            p++;
        }

        char *save_ptr;
        char *token = strtok_r(p, " \t\r\n", &save_ptr);

        while (token) {
            if (strstr(token, ".o")) {
                if (add_path(path, token) == 0) {
                    printf("OBJ_Y += %s\n", path);
                    truncate_path(path);
                }
            } else if (strchr(token, '/')) {
                if (add_path(path, token) == 0) {
                    parse(path);
                    truncate_path(path);
                }
            }

            token = strtok_r(NULL, " \t\r\n", &save_ptr);
        }
    }

    fclose(f);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "用法：%s <初始目录> <config文件>\n", argv[0]);
        return 1;
    }

    const char *arch = getenv("ARCH");
    if (arch != NULL && arch[0] != '\0') {
        selected_arch = arch;
    }

    if (load_config(argv[2]) < 0) {
        return 1;
    }

    char path[MAX_PATH_LEN];

    strncpy(path, argv[1], sizeof(path) - 1);
    path[sizeof(path) - 1] = '\0';

    parse(path);

    return 0;
}