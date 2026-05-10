#ifndef __OS_PIPE_H
#define __OS_PIPE_H

#include <os/types.h>
#include <os/spinlock.h>
#include <os/wait.h>

#define PIPE_BUF_SIZE 4096

struct pipe_inode_info {
    size_t head;
    size_t tail;
    size_t count;

    int readers;
    int writers;

    struct wait_queue_head read_wait;
    struct wait_queue_head write_wait;

    spinlock_t lock;

    char *buf;
};

#endif