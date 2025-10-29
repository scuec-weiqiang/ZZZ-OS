/**
 * @FilePath: /ZZZ/kernel/fs/vfs/vfs_path.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-09-01 00:21:01
 * @LastEditTime: 2025-09-07 17:18:51
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "os/types.h"
#include "os/check.h"
#include "os/string.h"


/**
 * @brief 将路径字符串分割成多个token
 *
 * 该函数用于将给定的路径字符串分割成多个token，每个token表示路径中的一个部分。
 * 如果传入的字符串为NULL，则继续从上次分割的位置开始分割。
 *
 * @param str 要分割的路径字符串，如果为NULL则继续上次分割的位置
 *
 * @return 返回下一个token的起始位置，如果没有更多token则返回NULL。
 */
char *path_split(char *str, const char *delim)
{
    static char *cur_token = NULL; // 用于保存下一个token的起始位置
    static char *end = NULL;       // 用于保存字符串结束位置

    if (str != NULL)
    {
        cur_token = str; // 保存当前token的起始位置
        end = str;
        // 如果str不为NULL，说明是第一次调用
        for (int i = 0; str[i] != '\0'; i++)
        {
            if (str[i] == delim[0]) // 找到路径分隔符
            {
                str[i] = '\0'; // 将分隔符替换为字符串结束符
            }
            end++; // 更新end指针，直到指向字符串的末尾
        }
    }
    else
    {
        // 如果str为NULL，说明是后续调用
        while (
            *cur_token !=
            '\0') // 这时cur_token指向上一个token的开头，需要跳过上一段token指向下一个/0分隔符
        {
            if (cur_token == end) // 如果
            {
                cur_token = NULL;
                break; // 如果已经到达字符串末尾，返回NULL
            }
            cur_token++;
        }
    }

    while (
        *cur_token ==
        '\0') // 这时cur_token已经不再指向上一个token的内容，但是有可能上个token结尾有多个/0分隔符，需要跳过这些分隔符指向下一个token开头
    {
        // 注意，一定要先判断是否到达字符串末尾，否则不会及时跳出循环，会越界报错
        if (cur_token == end)
        {
            cur_token = NULL;
            break; // 如果已经到达字符串末尾，返回NULL
        }
        cur_token++;
    }

    return cur_token;
}

/**
* @brief 将路径分割为目录名和文件名
*
* 该函数将给定的路径字符串分割为目录名和文件名，并分别存储在提供的两个字符数组中。
*
* @param path 输入的路径字符串
* @param dirname 用于存储目录名的字符数组
* @param basename 用于存储文件名的字符数组
*
* @return 返回0表示成功，返回-1表示失败
*/
int base_dir_split(const char *path,char *dirname, char *basename)
{
    CHECK(path != NULL, "", return -1;);
    CHECK(dirname != NULL, "", return -1;);
    CHECK(basename != NULL, "", return -1;);

    size_t len = strlen(path);
    if (len == 0) 
    {
        dirname[0] = '\0';
        basename[0] = '\0';
        return 0;
    }

    size_t tail = len;

    // 去掉末尾的 '/'
    while (tail > 1 && path[tail - 1] == '/') 
    {
        tail--;
    }

    // 向前查找最后一个 '/'
    size_t last_slash = (size_t)-1;
    for (size_t i = tail; i > 0; i--) 
    {
        if (path[i - 1] == '/') 
        {
            last_slash = i - 1;
            break;
        }
    }

    if (last_slash == (size_t)-1) 
    {
        // 没有斜杠
        strcpy(dirname, ".");                     // 当前目录
        strncpy(basename, path, tail);
        basename[tail] = '\0';
    } 
    else 
    {
        // dirname
        size_t dir_len = last_slash + 1;          // 包含 '/'
        strncpy(dirname, path, dir_len);
        dirname[dir_len] = '\0';

        // basename
        size_t base_len = tail - (last_slash + 1);
        strncpy(basename, path + last_slash + 1, base_len);
        basename[base_len] = '\0';
    }

    return 0;
}

