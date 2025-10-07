/**
 * @FilePath: /ZZZ/kernel/fs/ext2/ext2_erros.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-08-13 21:27:21
 * @LastEditTime: 2025-08-26 19:04:37
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#ifndef EXT2_ERROS_H
#define EXT2_ERROS_H

// ext2_errors.h
#ifndef EXT2_ERRORS_H
#define EXT2_ERRORS_H

#define EXT2_SUCCESS               0   // 操作成功

// 核心错误码
#define EXT2_ET_BAD_MAGIC          1   // 超级块魔数无效
#define EXT2_ET_SB_CORRUPTED       2   // 超级块损坏
#define EXT2_ET_GDESC_CORRUPTED    3   // 块组描述符损坏
#define EXT2_ET_INODE_CORRUPTED    4   // Inode 结构损坏
#define EXT2_ET_DIR_CORRUPTED      5   // 目录项损坏
#define EXT2_ET_BB_CORRUPT         6   // 块位图损坏
#define EXT2_ET_IB_CORRUPT         7   // Inode 位图损坏
#define EXT2_ET_BAD_INODE          8   // 无效的 inode 号
#define EXT2_ET_BAD_BLOCK          9   // 无效的块号
#define EXT2_ET_BLOCK_ALLOC_FAIL   10  // 块分配失败
#define EXT2_ET_INODE_ALLOC_FAIL   11  // Inode 分配失败

// 文件操作错误
#define EXT2_ET_FILE_NOT_FOUND     12  // 文件不存在
#define EXT2_ET_NOT_DIRECTORY      13  // 不是目录
#define EXT2_ET_IS_DIRECTORY       14  // 是目录（不适用于文件操作）
#define EXT2_ET_DIR_NOT_EMPTY      15  // 目录非空
#define EXT2_ET_EOF                16  // 到达文件末尾

// 资源限制
#define EXT2_ET_NO_SPACE           20  // 磁盘空间不足
#define EXT2_ET_TOO_MANY_LINKS     21  // 链接数过多（超过最大硬链接数）

// 权限错误
#define EXT2_ET_PERM_DENIED        30  // 权限不足
#define EXT2_ET_OWNER_MISMATCH     31  // 所有者不匹配

// 其他
#define EXT2_ET_UNSUPP_FEATURE     40  // 不支持的特性
#define EXT2_ET_MOUNTED_RDONLY     41  // 文件系统只读挂载
#define EXT2_ET_JOURNAL_ERR        42  // 日志错误

#endif // EXT2_ERRORS_H

#endif