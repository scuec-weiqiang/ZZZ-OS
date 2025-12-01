#include "os/list.h"
#include "os/pfn.h"
#include "os/types.h"
#include <os/mm/slab.h>
#include <os/mm/buddy.h>
#include <os/string.h>
#include <os/list.h>

#define get_slab(list_node) list_entry(list_node, struct slab, list)

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

struct kmem_cache* kmem_cache_create(const char *name, size_t size, size_t align) {
    if (!name) return NULL;

    struct kmem_cache *cache = alloc_page_kva();
    if (!cache) {
        return NULL;
    }

    strcpy(cache->name, name);
    cache->align = round_up_align(align, 8);
    cache->object_size = size;
    cache->objects_per_slab = (PAGE_SIZE - sizeof(struct slab)) / size;

    INIT_LIST_HEAD(&cache->free_slabs);
    INIT_LIST_HEAD(&cache->full_slabs);
    INIT_LIST_HEAD(&cache->partial_slabs);

    cache->total_slabs = 0;

    return cache;
}

static struct slab* init_slab(struct kmem_cache *cache) {
    void *mem = alloc_page_kva();
    struct slab *slab = (struct slab*)mem;
    slab->parent = cache;
    INIT_LIST_HEAD(&slab->list);
    list_add(&cache->free_slabs, &slab->list);

    struct free_obj* obj = (struct free_obj*)((phys_addr_t)slab + (phys_addr_t)sizeof(struct slab));
    slab->free_object.next = obj;
    struct free_obj* next = (struct free_obj*)((size_t)obj + cache->object_size);
    
    for (int i = 0; i < cache->objects_per_slab - 1; i++) {
        obj->next = next;
        obj = next;
        next =  (struct free_obj*)((size_t)obj + cache->object_size);
    }
    next->next = NULL;

    return slab;
}

void *kmem_cache_alloc(struct kmem_cache *cache) {
    struct slab *slab = NULL;
    struct list_head *node = NULL;

    if (!list_empty(&cache->partial_slabs)) {
       goto alloc_obj;
    }

    if (!list_empty(&cache->free_slabs)) {
       goto add_partial;
    }
    else {
        slab = init_slab(cache);
        list_add(&cache->free_slabs, &slab->list);
        cache->total_slabs++;
    }

    add_partial:
    node = cache->free_slabs.next;
    list_del(node);
    list_add(&cache->partial_slabs, node);

    alloc_obj:
    slab = get_slab(&cache->partial_slabs);
    slab->inuse++;
    if (slab->inuse >= cache->objects_per_slab) {
        list_del(&slab->list);
        list_add(&cache->full_slabs, &slab->list);
    } else {
        slab->free_object.next = ((struct free_obj*)slab->free_object.next)->next;
    }
  
    return slab->free_object.next;
}

void keme_cache_free(void *obj) {
    phys_addr_t slab_base = (phys_addr_t)obj & PAGE_MASK;
    struct slab *slab = (struct slab *)slab_base;
    struct free_obj *free_obj = (struct free_obj*)obj;
    free_obj->next = slab->free_object.next;
    slab->free_object.next = free_obj;
    slab->inuse--;

    struct kmem_cache *cache = slab->parent;
    if (slab->inuse == 0) {
        list_del(&slab->list);
        // list_add(&cache->free_slabs, &slab->list);
        free_page_kva(slab);
    }

    if (slab->inuse == cache->objects_per_slab - 1) {
        list_del(&slab->list);
        list_add(&cache->partial_slabs, &slab->list);
    }    

}

void slab_test() {

}