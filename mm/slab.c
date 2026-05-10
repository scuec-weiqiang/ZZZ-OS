#include <mm/buddy.h>
#include <mm/page.h>
#include <mm/physmem.h>
#include <mm/slab.h>
#include <os/kva.h>
#include <os/list.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/string.h>
#include <os/types.h>
#include <os/utils.h>

#define get_slab(list_node) list_entry(list_node, struct slab, list)

#define SIZE_TYPE_0 8
#define SIZE_TYPE_1 16
#define SIZE_TYPE_2 32
#define SIZE_TYPE_3 48
#define SIZE_TYPE_4 64
#define SIZE_TYPE_5 96
#define SIZE_TYPE_6 128
#define SIZE_TYPE_7 192
#define SIZE_TYPE_8 256
#define SIZE_TYPE_9 384
#define SIZE_TYPE_10 512
#define SIZE_TYPE_11 768
#define SIZE_TYPE_12 1024
#define SIZE_TYPE_13 1536
#define SIZE_TYPE_14 2048

static const int size_types[] = {
    SIZE_TYPE_0, SIZE_TYPE_1, SIZE_TYPE_2, SIZE_TYPE_3,
    SIZE_TYPE_4, SIZE_TYPE_5, SIZE_TYPE_6, SIZE_TYPE_7,
    SIZE_TYPE_8, SIZE_TYPE_9, SIZE_TYPE_10, SIZE_TYPE_11,
    SIZE_TYPE_12, SIZE_TYPE_13, SIZE_TYPE_14};

static struct {
    size_t size;
    struct kmem_cache *cache;
} kmalloc_caches[] = {
    {SIZE_TYPE_0, NULL}, {SIZE_TYPE_1, NULL}, {SIZE_TYPE_2, NULL}, {SIZE_TYPE_3, NULL}, {SIZE_TYPE_4, NULL}, {SIZE_TYPE_5, NULL}, {SIZE_TYPE_6, NULL}, {SIZE_TYPE_7, NULL}, {SIZE_TYPE_8, NULL}, {SIZE_TYPE_9, NULL}, {SIZE_TYPE_10, NULL}, {SIZE_TYPE_11, NULL}, {SIZE_TYPE_12, NULL}, {SIZE_TYPE_13, NULL}, {SIZE_TYPE_14, NULL}};

#define SLAB_SIZE_TYPES_NUM (sizeof(size_types) / sizeof(size_types[0]))
#define SLAB_MAX_SIZE 2048
#define SLAB_MIN_ALIGN 8

static inline int slab_debug_size(size_t size) {
    return size == 128 || size == 136 || size == 232;
}

static inline size_t slab_objects_offset(const struct kmem_cache *cache) {
    return ALIGN_UP(sizeof(struct slab), cache->align);
}

static inline uintptr_t slab_objects_start(const struct kmem_cache *cache,
                                           const struct slab *slab) {
    return (uintptr_t)slab + slab_objects_offset(cache);
}

static inline uintptr_t slab_objects_end(const struct kmem_cache *cache,
                                         const struct slab *slab) {
    return slab_objects_start(cache, slab) +
           (uintptr_t)cache->objects_per_slab * cache->object_size;
}

static int slab_obj_is_valid(const struct kmem_cache *cache,
                             const struct slab *slab,
                             const struct free_obj *obj) {
    uintptr_t start;
    uintptr_t end;
    uintptr_t addr;

    if (!obj) {
        return 1;
    }

    start = slab_objects_start(cache, slab);
    end = slab_objects_end(cache, slab);
    addr = (uintptr_t)obj;

    if (addr < start || addr >= end) {
        return 0;
    }

    return ((addr - start) % cache->object_size) == 0;
}

static void slab_panic_bad_obj(const char *tag,
                               const struct kmem_cache *cache,
                               const struct slab *slab,
                               const struct free_obj *obj) {
    panic("%s: cache=%xu obj_size=%xu slab=%xu obj=%xu start=%xu end=%xu inuse=%xu objs=%xu\n",
          tag, cache, cache->object_size, slab, obj,
          slab_objects_start(cache, slab), slab_objects_end(cache, slab),
          slab->inuse, cache->objects_per_slab);
}

static inline size_t round_up_align(size_t sz, size_t align) {
    return (sz + (align - 1)) & ~(align - 1);
}

static inline size_t cache_object_size(size_t size, size_t align) {
    if (size < sizeof(struct free_obj)) {
        size = sizeof(struct free_obj);
    }
    return round_up_align(size, align);
}

static int size_to_index(size_t size) {
    if (size == 0)
        return 0;
    size = round_up_align(size, SLAB_MIN_ALIGN);

    for (int i = 0; i < SLAB_SIZE_TYPES_NUM; i++) {
        if (size <= size_types[i])
            return i;
    }
    return -1;
}

struct kmem_cache *kmem_cache_create(const char *name, size_t size, size_t align) {
    if (!name)
        return NULL;

    struct kmem_cache *cache = alloc_pages_kva(1);
    if (!cache) {
        return NULL;
    }

    strcpy(cache->name, name);
    cache->align = round_up_align(align ? align : SLAB_MIN_ALIGN, SLAB_MIN_ALIGN);
    cache->object_size = cache_object_size(size, cache->align);

    size_t obj_offset = slab_objects_offset(cache);
    if (obj_offset >= PAGE_SIZE) {
        free_pages_kva(cache);
        return NULL;
    }

    cache->objects_per_slab = div_u32(PAGE_SIZE - obj_offset, cache->object_size);
    if (cache->objects_per_slab == 0) {
        free_pages_kva(cache);
        return NULL;
    }

    INIT_LIST_HEAD(&cache->free_slabs);
    INIT_LIST_HEAD(&cache->full_slabs);
    INIT_LIST_HEAD(&cache->partial_slabs);
    spin_lock_init(&cache->lock);

    cache->total_slabs = 0;

    return cache;
}

static struct slab *init_slab(struct kmem_cache *cache) {
    void *mem = alloc_pages_kva(1);
    if (!mem) {
        return NULL;
    }

    struct slab *slab = (struct slab *)mem;
    slab->parent = cache;
    slab->inuse = 0;
    INIT_LIST_HEAD(&slab->list);
    list_add(&cache->free_slabs, &slab->list);

    struct free_obj *obj = (struct free_obj *)((size_t)slab + slab_objects_offset(cache));
    slab->free_object.next = obj;

    for (unsigned int i = 0; i < cache->objects_per_slab - 1; i++) {
        struct free_obj *next = (struct free_obj *)((size_t)obj + cache->object_size);
        obj->next = next;
        obj = next;
    }
    obj->next = NULL;

    struct page *pg = address_page(mem);
    pg->slab = slab;
    slab->magic = SLAB_MAGIC;

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
    } else {
        slab = init_slab(cache);
        if (!slab) {
            return NULL;
        }
        cache->total_slabs++;
    }

add_partial:
    node = cache->free_slabs.next;
    list_del(node);
    list_add(&cache->partial_slabs, node);

alloc_obj:
    slab = get_slab(cache->partial_slabs.next);
    struct free_obj *obj = slab->free_object.next;
    phys_addr_t ret;

    if (!obj) {
        panic("kmem_cache_alloc: slab free list is empty");
    }
    if (!slab_obj_is_valid(cache, slab, obj)) {
        slab_panic_bad_obj("kmem_cache_alloc: invalid free head", cache, slab, obj);
    }

    slab->free_object.next = obj->next;
    if (!slab_obj_is_valid(cache, slab, slab->free_object.next)) {
        slab_panic_bad_obj("kmem_cache_alloc: invalid next head", cache, slab,
                           slab->free_object.next);
    }
    slab->inuse++;
    ret = (phys_addr_t)obj;
    if (slab->inuse >= cache->objects_per_slab) {
        list_del(&slab->list);
        list_add(&cache->full_slabs, &slab->list);
    }

    return (void *)ret;
}

void kmem_cache_free(void *obj) {
    if (!obj)
        return;

    phys_addr_t slab_base = (phys_addr_t)obj & PAGE_MASK;
    struct slab *slab = (struct slab *)slab_base;
    if (slab->magic != SLAB_MAGIC) {
        panic("kmem_cache_free: invalid slab magic");
    }
    struct free_obj *free_obj = (struct free_obj *)obj;
    if (!slab_obj_is_valid(slab->parent, slab, free_obj)) {
        slab_panic_bad_obj("kmem_cache_free: invalid object", slab->parent, slab, free_obj);
    }
    free_obj->next = slab->free_object.next;
    slab->free_object.next = free_obj;
    slab->inuse--;

    struct kmem_cache *cache = slab->parent;
    if (slab->inuse == 0) {
        list_del(&slab->list);
        slab->magic = 0;
        cache->total_slabs--;
        free_pages_kva(slab);
        return;
    }

    if (slab->inuse == cache->objects_per_slab - 1) {
        list_del(&slab->list);
        list_add(&cache->partial_slabs, &slab->list);
    }
}

void *__kmalloc(size_t size) {
    void *ptr;

    if (size == 0)
        return NULL;

    if (size > SLAB_MAX_SIZE) {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        ptr = alloc_pages_kva(npages);
        if (slab_debug_size(size)) {
            printk("kmalloc large: req=%xu pages=%xu ptr=%xu\n",
                   size, npages, ptr);
        }
        return ptr;
    }

    int idx = size_to_index(size);
    if (idx < 0)
        return NULL;

    ptr = kmem_cache_alloc(kmalloc_caches[idx].cache);
    // if (slab_debug_size(size) && ptr) {
    //     struct kmem_cache *cache = kmalloc_caches[idx].cache;
    //     printk("kmalloc slab: req=%xu cache=%xu obj=%xu ptr=%xu slab=%xu inuse=%xu total=%xu\n",
    //            size, cache, cache->object_size, ptr,
    //            (void *)((phys_addr_t)ptr & PAGE_MASK),
    //            ((struct slab *)((phys_addr_t)ptr & PAGE_MASK))->inuse,
    //            cache->total_slabs);
    // }
    return ptr;
}

void __kfree(void *ptr) {
    if (!ptr)
        return;

    // 只有按页对齐且对应的page有slab信息，才是从slab分配的，否则是从buddy分配的
    if (IS_ALIGNED((uintptr_t)ptr, PAGE_SIZE)) {
        pfn_t pfn = phys_to_pfn(KERNEL_PA(ptr));
        struct page *pg = pfn_to_page(pfn);
        if (!pg->slab) {
            free_pages_kva(ptr);
            return;
        }
    }

    kmem_cache_free(ptr);
}

void slab_init() {
    for (int i = 0; i < SLAB_SIZE_TYPES_NUM; i++) {
        kmalloc_caches[i].cache = kmem_cache_create("kmalloc_cache", kmalloc_caches[i].size, SLAB_MIN_ALIGN);
        if (!kmalloc_caches[i].cache) {
            panic("__kmalloc cache create failed");
        }
    }

    extern struct task_struct *alloc_task_struct_init(void);
    if (alloc_task_struct_init() < 0) {
        panic("task_struct cache create failed");
    }

    extern int alloc_files_struct_init(void);
    if (alloc_files_struct_init() < 0) {
        panic("files_struct cache create failed");
    }

    extern int alloc_fs_struct_init(void);
    if (alloc_fs_struct_init() < 0) {
        panic("fs_struct cache create failed");
    }

    extern int alloc_file_init(void);
    if (alloc_file_init() < 0) {
        panic("file cache create failed");
    }

    extern int alloc_pipe_inode_info_init(void);
    if (alloc_pipe_inode_info_init() < 0) {
        panic("pipe cache create failed");
    }
}

// #include <os/mm.h>
// #include <os/rand.h>

// static void kmalloc_basic_test(void) {
//     printk("[kmalloc_basic_test] start\n");

//     for (int i = 0; i < 1000; i++) {
//         size_t sz = (i % 128) + 1;
//         void *p = __kmalloc(sz);

//         if (!p)
//             panic("__kmalloc failed at %d", i);

//         memset(p, 0xAB, sz);
//         __kfree(p);
//     }

//     printk("[kmalloc_basic_test] PASS\n");
// }

// static void kmalloc_stress_test(void) {
//     #define MAX_PTRS 4096

//     static void *ptrs[MAX_PTRS];
//     static size_t sizes[MAX_PTRS];

//     printk("[kmalloc_stress_test] start\n");

//     for (int i = 0; i < MAX_PTRS; i++) {
//         ptrs[i] = NULL;
//         sizes[i] = 0;
//     }

//     /* 随机分配 */
//     for (int i = 0; i < MAX_PTRS; i++) {
//         size_t sz = (rand() % 8000) + 1;  // 1~8KB

//         ptrs[i] = __kmalloc(sz);
//         sizes[i] = sz;

//         if (!ptrs[i]) {
//             panic("__kmalloc failed at %d size=%d", i, sz);
//         }

//         memset(ptrs[i], 0x5A, sz);
//     }

//     /* 随机顺序释放 */
//     for (int i = 0; i < MAX_PTRS; i++) {
//         int idx = rand() % MAX_PTRS;

//         if (ptrs[idx]) {
//             __kfree(ptrs[idx]);
//             ptrs[idx] = NULL;
//         }
//     }

//     /* 确保全部释放 */
//     for (int i = 0; i < MAX_PTRS; i++) {
//         if (ptrs[i]) {
//             __kfree(ptrs[i]);
//         }
//     }

//     printk("[kmalloc_stress_test] PASS\n");
// }

// static void buddy_contiguous_test(void) {
//     printk("[buddy_contiguous_test] start\n");

//     for (int order = 0; order <= 5; order++) {
//         void *p = __kmalloc(PAGE_SIZE << order);
//         buddy_dump();
//         if (!p)
//         {

//             panic("alloc failed for order %d", order);
//         }

//         /* 测试页是否连续 */
//         for (int i = 0; i < (1 << order); i++) {
//             void *cur = (void *)((uintptr_t)p + i * PAGE_SIZE);
//             void *next = (void *)((uintptr_t)p + (i+1) * PAGE_SIZE);

//             if (i + 1 < (1 << order)) {
//                 if (KERNEL_PA(next) != KERNEL_PA(cur) + PAGE_SIZE) {
//                     panic("pages not contiguous for order %d", order);
//                 }
//             }
//         }

//         __kfree(p);
//     }

//     printk("[buddy_contiguous_test] PASS\n");
// }

// static void slab_test() {
//     printk("\n==== SLAB TEST BEGIN ====\n");

//     /* 1. 创建缓存 */
//     struct kmem_cache *cache = kmem_cache_create("test_cache", 32, 8);
//     if (!cache) {
//         printk("[SLAB TEST] cache create failed\n");
//         return;
//     }

//     printk("[SLAB TEST] cache created, obj_size=%d, per_slab=%d\n",
//            (int)cache->object_size,
//            (int)cache->objects_per_slab);

//     /* 2. 简单分配测试 */
//     void *a = kmem_cache_alloc(cache);
//     void *b = kmem_cache_alloc(cache);
//     void *c = kmem_cache_alloc(cache);

//     printk("[SLAB TEST] alloc a=%xu b=%xu c=%xu\n", a, b, c);

//     if (!a || !b || !c) {
//         panic("basic alloc failed");
//     }

//     memset(a, 0xAA, 32);
//     memset(b, 0xBB, 32);
//     memset(c, 0xCC, 32);

//     kmem_cache_free(a);
//     kmem_cache_free(b);
//     kmem_cache_free(c);

//     printk("[SLAB TEST] basic kfree ok\n");

//     /* 3. 打满 slab */
//     int n = cache->objects_per_slab;
//     printk("[SLAB TEST] filling one slab: %d objects\n", n);

//     void **objs = alloc_pages_kva(1);
//     for (int i = 0; i < n; i++) {
//         objs[i] = kmem_cache_alloc(cache);
//         if (!objs[i]) {
//             panic("fill slab failed");
//         }
//         memset(objs[i], i, 32);
//     }

//     printk("[SLAB TEST] slab filled\n");

//     /* 4. 再多分配几个，触发新 slab */
//     void *extra1 = kmem_cache_alloc(cache);
//     void *extra2 = kmem_cache_alloc(cache);

//     printk("[SLAB TEST] extra slabs: %xu %xu\n", extra1, extra2);

//     if (!extra1 || !extra2) {
//         panic("second slab alloc failed");
//     }

//     /* 5. 全部释放 */
//     for (int i = 0; i < n; i++) {
//         kmem_cache_free(objs[i]);
//     }

//     kmem_cache_free(extra1);
//     kmem_cache_free(extra2);

//     printk("[SLAB TEST] full release done\n");

//     free_pages_kva(objs);

//     /* 6. 随机压力测试 */
//     printk("[SLAB TEST] stress test begin\n");

//     void *ptrs[256];
//     memset(ptrs, 0, sizeof(ptrs));

//     for (int i = 0; i < 10000; i++) {
//         int idx = i % 256;

//         if (ptrs[idx] == NULL) {
//             ptrs[idx] = kmem_cache_alloc(cache);
//             if (!ptrs[idx]) {
//                 panic("stress alloc failed");
//             }
//             memset(ptrs[idx], 0x5A, 32);
//         } else {
//             kmem_cache_free(ptrs[idx]);
//             ptrs[idx] = NULL;
//         }
//     }

//     for (int i = 0; i < 256; i++) {
//         if (ptrs[i]) {
//             kmem_cache_free(ptrs[i]);
//         }
//     }
//     free_pages_kva((void*)cache);
//     printk("==== SLAB TEST PASSED ====\n");

//     slab_init();
//     // kmalloc_basic_test();
//     // kmalloc_stress_test();
//     // buddy_contiguous_test();

//     printk("\n __kmalloc system test finish \n");
// }
