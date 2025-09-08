/**
 * @FilePath: /ZZZ/kernel/buf.h
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-24 17:09:49
 * @LastEditTime: 2025-09-07 20:08:06
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
// /**
//  * @FilePath: /ZZZ/kernel/buf.h
//  * @Description:  
//  * @Author: scuec_weiqiang scuec_weiqiang@qq.com
//  * @Date: 2025-05-24 17:09:49
//  * @LastEditTime: 2025-05-24 17:16:16
//  * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
//  * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
// */
// #ifndef BUF_H
// #define BUF_H

// #include "types.h"
// #include "spinlock.h"

// struct buf {
//   int64_t valid;   // has data been read from disk?
//   int64_t disk;    // does disk "own" buf?
//   uint64_t dev;
//   uint64_t blockno;
//   // struct sleeplock lock;
//   spinlock_t lock;
//   uint64_t refcnt;
//   struct buf *prev; // LRU cache list
//   struct buf *next;
//   uint8_t data[4096];
// };

// #endif
