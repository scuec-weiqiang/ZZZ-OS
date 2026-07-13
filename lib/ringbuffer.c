#include <os/pfn.h>
#include <os/ringbuffer.h>


#include <os/kmalloc.h>
#define __RB_MALLOC(size) (struct ringbuffer*)kmalloc(size)
#define __RB_FREE(ptr) do{kfree(ptr);ptr=NULL;}while(0)

#define __BUF_MALLOC(npages) (char*)page_alloc(npages)
#define __BUF_FREE(buf) __RB_FREE(buf)

void ringbuffer_reset(struct ringbuffer *rb) {
    if (!rb)
        return;

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

struct ringbuffer* ringbuffer_alloc(size_t capacity) {
    if (capacity == 0) return NULL;

    struct ringbuffer *rb = __RB_MALLOC(sizeof(*rb));
    if (!rb) return NULL;

    ringbuffer_reset(rb);
    rb->capacity = capacity;

    size_t npages = (capacity + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
    rb->buffer = __BUF_MALLOC(npages);

    if (!rb->buffer) {
        __RB_FREE(rb);
        return NULL;
    }

    return rb;
}

void ringbuffer_free(struct ringbuffer *rb) {
    if (!rb) return;
    __BUF_FREE(rb->buffer);
    __RB_FREE(rb);
}

int ringbuffer_init(struct ringbuffer *rb, void *buffer, size_t capacity){
    if (!rb || !buffer || capacity == 0) return -1;
    rb->capacity = capacity;
    rb->buffer = buffer;
    ringbuffer_reset(rb);
    return 0;
}

bool ringbuffer_empty(const struct ringbuffer *rb) {
    return rb->count == 0 ? true : false;
}

bool ringbuffer_full(const struct ringbuffer *rb) {
    return rb->count >= rb->capacity ? true : false;
}

size_t ringbuffer_count(const struct ringbuffer *rb) {
    return rb->count;
}

size_t ringbuffer_space(const struct ringbuffer *rb) {
    return rb->capacity - rb->count;
}

bool ringbuffer_put(struct ringbuffer *rb, unsigned char ch) {
    if (!rb || rb->count == rb->capacity)
        return false;

    rb->buffer[rb->head] = ch;

    rb->head++;
    if (rb->head == rb->capacity)
        rb->head = 0;

    rb->count++;

    return true;
}

bool ringbuffer_get(struct ringbuffer *rb, unsigned char *ch) {
    if (!rb || !ch || rb->count == 0)
        return false;

    *ch = rb->buffer[rb->tail];

    rb->tail++;
    if (rb->tail == rb->capacity)
        rb->tail = 0;

    rb->count--;

    return true;
}

bool ringbuffer_peek(const struct ringbuffer *rb, unsigned char *ch) {
    if (!rb || !ch || rb->count == 0)
        return false;

    *ch = rb->buffer[rb->tail];
    return true;
}

size_t ringbuffer_write(struct ringbuffer *rb, const void *data, size_t length) {
    const unsigned char *src = data;
    size_t written = 0;

    if (!rb || !src)
        return 0;

    while (written < length) {
        if (!ringbuffer_put(rb, src[written]))
            break;
        written++;
    }

    return written;
}

size_t ringbuffer_read(struct ringbuffer *rb, void *data, size_t length) {
    unsigned char *dst = data;
    size_t read_count = 0;

    if (!rb || !dst)
        return 0;

    while (read_count < length) {
        if (!ringbuffer_get(rb, &dst[read_count]))
            break;

        read_count++;
    }

    return read_count;
}