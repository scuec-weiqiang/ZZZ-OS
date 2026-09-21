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
#include <os/errno.h>

_Static_assert(sizeof(struct kmem_cache) <= PAGE_SIZE, "kmem_cache exceeds its allocation");

#define get_slab(list_node) list_entry(list_node, struct slab, list)
#define obj_to_slab(obj) ((struct slab*)((addr_t)(obj) & PAGE_MASK))

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
    {SIZE_TYPE_0, NULL}, {SIZE_TYPE_1, NULL}, 
    {SIZE_TYPE_2, NULL}, {SIZE_TYPE_3, NULL}, 
    {SIZE_TYPE_4, NULL}, {SIZE_TYPE_5, NULL}, 
    {SIZE_TYPE_6, NULL}, {SIZE_TYPE_7, NULL}, 
    {SIZE_TYPE_8, NULL}, {SIZE_TYPE_9, NULL}, 
    {SIZE_TYPE_10, NULL}, {SIZE_TYPE_11, NULL}, 
    {SIZE_TYPE_12, NULL}, {SIZE_TYPE_13, NULL},
    {SIZE_TYPE_14, NULL}};

#define SLAB_SIZE_TYPES_NUM (sizeof(size_types) / sizeof(size_types[0]))
#define SLAB_MAX_SIZE 2048
#define SLAB_MIN_ALIGN 8

static inline int slab_debug_size(size_t size) {
    return size == 128 || size == 136 || size == 232;
}

// slab后什么位置开始obj
static inline size_t slab_objects_offset(const struct kmem_cache *cache) {
    return ALIGN_UP(sizeof(struct slab), cache->align);
}

/*
之前计算obj开始位置时用 ALIGN_UP(slab + sizeof(struct slab), cache->object_size);
没考虑到有部分object不是按2的次幂对齐的，导致后面部分object超出本页范围
这里统一用这个函数算
*/
static inline uintptr_t slab_objects_start(const struct kmem_cache *cache,
                                           const struct slab *slab) 
{
    return (uintptr_t)slab + slab_objects_offset(cache);
}

static inline uintptr_t slab_objects_end(const struct kmem_cache *cache,
                                         const struct slab *slab) 
{
    return slab_objects_start(cache, slab) +
           (uintptr_t)cache->objects_per_slab * cache->object_size;
}

static int slab_obj_is_valid(const struct kmem_cache *cache,
                             const struct slab *slab,
                             const struct free_obj *obj) 
{
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
                               const struct free_obj *obj);

// fix: bug太多了。。。之前没有对double free的情况进行检查
// free obj前在链表里遍历，检查obj是否已经在freelist里，防止成环
static int slab_obj_is_free(const struct kmem_cache *cache,
                            const struct slab *slab,
                            const struct free_obj *obj) 
{
    const struct free_obj *cur;
    unsigned int seen = 0;

    for (cur = slab->free_object.next; cur != NULL; cur = cur->next) {
        if (!slab_obj_is_valid(cache, slab, cur)) {
            slab_panic_bad_obj("kmem_cache_free: invalid object on free list",
                               cache, slab, cur);
        }

        if (cur == obj) {
            return 1;
        }

        // 防止已经成环了，导致无法跳出
        if (++seen > cache->objects_per_slab) {
            panic("kmem_cache_free: free list loop cache=%xu slab=%xu obj_size=%xu\n",
                  cache, slab, cache->object_size);
        }
    }

    return 0;
}

static void slab_panic_bad_obj(const char *tag,
                               const struct kmem_cache *cache,
                               const struct slab *slab,
                               const struct free_obj *obj) 
{
    panic("%s: cache=%xu obj_size=%xu slab=%xu obj=%xu start=%xu end=%xu inuse=%xu objs=%xu\n",
          tag, cache, cache->object_size, slab, obj,
          slab_objects_start(cache, slab), slab_objects_end(cache, slab),
          slab->inuse, cache->objects_per_slab);
}

static inline size_t round_up_align(size_t sz, size_t align) 
{
    return (sz + (align - 1)) & ~(align - 1);
}

static inline size_t cache_object_size(size_t size, size_t align) 
{
    if (size < sizeof(struct free_obj)) {
        size = sizeof(struct free_obj);
    }
    return round_up_align(size, align);
}

static int size_to_index(size_t size) 
{
    if (size == 0)
        return 0;
    size = round_up_align(size, SLAB_MIN_ALIGN);

    for (int i = 0; i < SLAB_SIZE_TYPES_NUM; i++) {
        if (size <= size_types[i])
            return i;
    }
    return -1;
}

struct kmem_cache *kmem_cache_create(const char *name, size_t size, size_t align) 
{
    if (!name)
        return NULL;

    struct kmem_cache *cache = alloc_pages_kva(1);
    if (!cache) {
        return NULL;
    }

    strcpy(cache->name, name);
    cache->align = round_up_align(align ? align : SLAB_MIN_ALIGN, SLAB_MIN_ALIGN);
    cache->object_size = cache_object_size(size, cache->align);

    /*obj起始位置*/
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

    for (int i = 0; i < MAX_CPUS; i++) {
        spin_lock_init(&cache->cpu[i].lock);
        cache->cpu[i].count = 0;
        cache->cpu[i].drains = 0;
        cache->cpu[i].refills = 0;
        cache->cpu[i].hits = 0;
        cache->cpu[i].misses = 0;
    }

    cache->total_slabs = 0;
    cache->mag_limit = MAG_SIZE;
    cache->mag_batch = MAG_SIZE / 2;
    for (int i = 0; i < MAX_CPUS; i++)
        cache->cpu[i].global_accesses = 0;

    return cache;
}

// static struct slab *init_slab(struct kmem_cache *cache) 
// {
//     void *mem = alloc_pages_kva(1);
//     if (!mem) {
//         return NULL;
//     }

//     struct slab *slab = (struct slab *)mem;
//     slab->parent = cache;
//     slab->inuse = 0;
//     INIT_LIST_HEAD(&slab->list);
//     list_add(&cache->free_slabs, &slab->list);

//     struct free_obj *obj = (struct free_obj *)((size_t)slab + slab_objects_offset(cache));
//     slab->free_object.next = obj;

//     for (unsigned int i = 0; i < cache->objects_per_slab - 1; i++) {
//         struct free_obj *next = (struct free_obj *)((size_t)obj + cache->object_size);
//         obj->next = next;
//         obj = next;
//     }

//     /*忘了加这个导致分配多了后直接崩了*/
//     obj->next = NULL;

//     struct page *pg = address_page(mem);
//     pg->slab = slab;
//     slab->magic = SLAB_MAGIC;

//     return slab;
// }

/* 现在锁有点复杂了，涉及到 cpu_cache->lock
                            → cache->lock
                                → PCP lock / buddy lock 
把 init_slab拆成两函数，alloc_slab_detached只负责分配，把加入freelist链表部分单独拆出来上锁
现在进入alloc_slab_detached前只可能持有cpu_cache->lock，函数内部可能会持有pcp/buddy的锁
*/
static struct slab *alloc_slab_detached(struct kmem_cache *cache)
{
    void *mem = alloc_pages_kva(1);
    if (mem == NULL)
        return NULL;

    struct slab *slab = mem;

    slab->parent = cache;
    slab->inuse = 0;
    slab->magic = SLAB_MAGIC;
    INIT_LIST_HEAD(&slab->list);

    /* 初始化 object freelist */
    struct free_obj *obj =
        (struct free_obj *)((uintptr_t)slab +
                            slab_objects_offset(cache));

    slab->free_object.next = obj;

    for (unsigned int i = 0;
         i < cache->objects_per_slab - 1;
         i++) {
        struct free_obj *next =
            (struct free_obj *)((uintptr_t)obj +
                                cache->object_size);

        obj->next = next;
        obj = next;
    }

    obj->next = NULL;

    struct page *page = address_page(mem);
    page->slab = slab;

    return slab;
}

static void add_slab_locked(struct kmem_cache *cache, struct slab *slab)
{
    list_add(&cache->free_slabs, &slab->list);
    cache->total_slabs++;
}

/*进入函数前持有的锁为：
   cpu_cache->lock
        → cache->lock
*/
static void* __kmem_cache_alloc_locked(struct kmem_cache *cache) 
{
    struct slab *slab = NULL;
    struct list_head *node = NULL;
    /*优先从没满的slab里分配*/
    if (!list_empty(&cache->partial_slabs)) {
        goto alloc_obj;
    } else if (!list_empty(&cache->free_slabs)) {
        /*slab都是满的那就看看有没有缓存的free slab*/
        goto add_partial;
    } else {
        /* 需要分配slab，这一步放到外面做方便加锁*/
        return NULL;
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
    /*保险起见还是校验一下*/
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

// 释放obj，如果产生了需要回收的 slab，那就返回这个free slab
static struct slab* __kmem_cache_free_locked(struct kmem_cache *cache, void *obj) 
{
    if (!obj)
        return NULL;

    struct slab *slab = obj_to_slab(obj);
    if (slab->magic != SLAB_MAGIC) {
        panic("kmem_cache_free: invalid slab magic");
    }
    struct free_obj *free_obj = (struct free_obj *)obj;
    if (!slab_obj_is_valid(slab->parent, slab, free_obj)) {
        slab_panic_bad_obj("kmem_cache_free: invalid object", slab->parent, slab, free_obj);
    }
    if (slab_obj_is_free(slab->parent, slab, free_obj)) {
        slab_panic_bad_obj("kmem_cache_free: double free", slab->parent, slab, free_obj);
    }
    free_obj->next = slab->free_object.next;
    slab->free_object.next = free_obj;
    slab->inuse--;

    if (slab->inuse == 0) {
        list_del(&slab->list);
        slab->magic = 0;
        cache->total_slabs--;
        return slab;
    }

    if (slab->inuse == cache->objects_per_slab - 1) {
        list_del(&slab->list);
        list_add(&cache->partial_slabs, &slab->list);
    }

    return NULL;
}

/*进入函数前持有的锁为：
   cpu_cache->lock
*/
static void refill_magazine(struct kmem_cache *cache,
                            struct kmem_cache_cpu *cpu_cache)
{
    unsigned int target = cache->mag_batch;
    unsigned int before = cpu_cache->count;

    while (cpu_cache->count < target) {
        void *obj;
        unsigned long flags;

        spin_lock_irqsave(&cache->lock, &flags);
        cpu_cache->global_accesses++;
        while (cpu_cache->count < target) {
            obj = __kmem_cache_alloc_locked(cache);
            // 如果obj为null，说明需要重新分配一个slab
            if (obj == NULL)
                break;
            cpu_cache->objects[cpu_cache->count++] = obj;
        }

        spin_unlock_irqrestore(&cache->lock, flags);

        if (cpu_cache->count >= target)
            break;

        // 没有可用 slab,此处已经释放 cache->lock，可以安全申请页面。
        struct slab *new_slab = alloc_slab_detached(cache);
        if (new_slab == NULL)
            break;

        spin_lock_irqsave(&cache->lock, &flags);
        cpu_cache->global_accesses++;
        add_slab_locked(cache, new_slab);
        spin_unlock_irqrestore(&cache->lock, flags);
    }
    if (cpu_cache->count > before)
        cpu_cache->refills++;
}

void drain_magazine(struct kmem_cache *cache, struct kmem_cache_cpu *cpu_cache) 
{
    struct slab *to_free[MAG_SIZE]; // 用来收集可能需要回收的slab
    unsigned int nr_slabs = 0;
    unsigned int nr_objects = cache->mag_batch;
    unsigned long flags;

    spin_lock_irqsave(&cache->lock, &flags);
    cpu_cache->global_accesses++;
    if (cpu_cache->count)
        cpu_cache->drains++;

    while (nr_objects-- && cpu_cache->count != 0) {
        void *obj = cpu_cache->objects[--cpu_cache->count];
        cpu_cache->objects[cpu_cache->count] = NULL;
        struct slab *empty = __kmem_cache_free_locked(cache, obj);
        if (empty) {
            to_free[nr_slabs++] = empty;
        }
    }
    spin_unlock_irqrestore(&cache->lock, flags);
  
    // 此时不再持有 cache->lock
    for (unsigned int i = 0; i < nr_slabs; i++)
        free_pages_kva(to_free[i]);
}


void *kmem_cache_alloc(struct kmem_cache *cache) 
{
    preempt_disable();
    struct kmem_cache_cpu *cpu_cache = &cache->cpu[get_cpuid()];
    unsigned long flags;
    spin_lock_irqsave(&cpu_cache->lock, &flags);

    if (cpu_cache->count != 0) {
        void *obj = cpu_cache->objects[--cpu_cache->count];
        cpu_cache->hits++;
        spin_unlock_irqrestore(&cpu_cache->lock, flags);
        preempt_enable();
        return obj;
    }

    cpu_cache->misses++;
    // 空了才执行批量 refill
    refill_magazine(cache, cpu_cache);

    void *obj = cpu_cache->count ?
      cpu_cache->objects[--cpu_cache->count] : NULL;
    spin_unlock_irqrestore(&cpu_cache->lock, flags);
    preempt_enable();
    return obj;
}

void kmem_cache_free(void* obj) 
{
    if (!obj)
        return;
    
    phys_addr_t slab_base = (phys_addr_t)obj & PAGE_MASK;
    struct slab *slab = (struct slab *)slab_base;
    if (slab->magic != SLAB_MAGIC) {
        panic("kmem_cache_free: invalid slab magic");
    }
    struct kmem_cache *cache = slab->parent;
    preempt_disable();
    struct kmem_cache_cpu *cpu_cache  = &cache->cpu[get_cpuid()];

    unsigned long flags;
    spin_lock_irqsave(&cpu_cache->lock, &flags);

    for (int i = 0; i < cpu_cache->count; i++) {
        if (cpu_cache->objects[i] == obj)
            panic("magazine double free");
    }
    if (cpu_cache->count >= cache->mag_limit) {
        drain_magazine(cache, cpu_cache);
    }
    cpu_cache->objects[cpu_cache->count++] = obj;

    spin_unlock_irqrestore(&cpu_cache->lock, flags);
    preempt_enable();
    return;
}

void kmem_cache_drain(struct kmem_cache *cache)
{
    for (int cpu = 0; cpu < MAX_CPUS; cpu++) {
        unsigned long flags;
        struct kmem_cache_cpu *local = &cache->cpu[cpu];
        spin_lock_irqsave(&local->lock, &flags);
        while (local->count)
            drain_magazine(cache, local);
        spin_unlock_irqrestore(&local->lock, flags);
    }

    /* 并发 refill 可能留下尚未分配过对象的空 slab */
    for (;;) {
        unsigned long flags;
        spin_lock_irqsave(&cache->lock, &flags);
        if (list_empty(&cache->free_slabs)) {
            spin_unlock_irqrestore(&cache->lock, flags);
            break;
        }
        struct slab *slab = get_slab(cache->free_slabs.next);
        list_del(&slab->list);
        slab->magic = 0;
        cache->total_slabs--;
        spin_unlock_irqrestore(&cache->lock, flags);
        free_pages_kva(slab);
    }
}

int kmem_cache_set_magazine(struct kmem_cache *cache, u32 limit, u32 batch)
{
    if (!cache || !batch || batch > limit || limit > MAG_SIZE)
        return -EINVAL;
    kmem_cache_drain(cache);
    cache->mag_limit = limit;
    cache->mag_batch = batch;
    return 0;
}


void *__kmalloc(size_t size) 
{
    void *ptr;

    if (size == 0)
        return NULL;

    if (size > SLAB_MAX_SIZE) {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        ptr = alloc_pages_kva(npages);
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

void __kfree(void *ptr) 
{
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

void slab_init() 
{
    for (int i = 0; i < SLAB_SIZE_TYPES_NUM; i++) {
        kmalloc_caches[i].cache = 
            kmem_cache_create("kmalloc_cache", kmalloc_caches[i].size, SLAB_MIN_ALIGN);
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
