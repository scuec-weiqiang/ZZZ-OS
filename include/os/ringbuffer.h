#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <os/types.h>


struct ringbuffer {
    size_t head;
    size_t tail;

    size_t capacity; /*of the buffer*/
    size_t count; /*of the buffer*/

    char* buffer;
};

// 动态内存分配，自动分配与释放数据结构与缓冲区
struct ringbuffer *ringbuffer_alloc(size_t capacity);
void ringbuffer_free(struct ringbuffer *rb);

// 静态初始化，数据结构和缓冲区可以在外部定义，调用者手动释放
int ringbuffer_init(struct ringbuffer *rb,
                    void *buffer,
                    size_t capacity);

void ringbuffer_reset(struct ringbuffer *rb);

bool ringbuffer_empty(const struct ringbuffer *rb);
bool ringbuffer_full(const struct ringbuffer *rb);

size_t ringbuffer_count(const struct ringbuffer *rb);
size_t ringbuffer_space(const struct ringbuffer *rb);

bool ringbuffer_put(struct ringbuffer *rb, unsigned char ch);
bool ringbuffer_get(struct ringbuffer *rb, unsigned char *ch);
bool ringbuffer_peek(const struct ringbuffer *rb, unsigned char *ch);

size_t ringbuffer_write(struct ringbuffer *rb,
                         const void *data,
                         size_t length);

size_t ringbuffer_read(struct ringbuffer *rb,
                        void *data,
                        size_t length);

#endif /* __RINGBUFFER_H__ */