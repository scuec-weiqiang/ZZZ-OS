#include <os/mm/slab.h>

static const int slab_size_types[] = {
    8, 16, 32, 48, 64, 
    96, 128, 192, 256, 384, 
    512, 768, 1024, 1536, 2048
};

#define SLAB_SIZE_TYPES_NUM (sizeof(slab_size_types)/sizeof(slab_size_types[0]))
#define SLAB_MAX_SIZE 2048
#define SLAB_MIN_ALIGN 8

static inline size_t round_up_align(size_t sz, size_t align) {
    return (sz + (align - 1)) & ~(align - 1);
}

static int size_to_index(size_t size) {
    if (size == 0) return 0;
    size = round_up_align(size, SLAB_MIN_ALIGN);

    for (int i = 0; i < SLAB_SIZE_TYPES_NUM; i++) {
        if (size <= slab_size_types[i]) return i;
    }
    return -1;
}

static size_t index_to_size(int idx) {
    if (idx < 0 || idx >= (int)SLAB_SIZE_TYPES_NUM) return 0;
    return round_up_align(slab_size_types[idx], SLAB_MIN_ALIGN);
}
