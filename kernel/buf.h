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
//   int valid;   // has data been read from disk?
//   int disk;    // does disk "own" buf?
//   u64 dev;
//   u64 blockno;
//   // struct sleeplock lock;
//   struct spinlock lock;
//   u64 refcnt;
//   struct buf *prev; // LRU cache list
//   struct buf *next;
//   u8 data[4096];
// };

// #endif
