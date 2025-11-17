/***************************************************************
 * @Author: weiqiang scuec_weiqiang@qq.com
 * @Date: 2024-10-16 11:39:24
 * @LastEditors: weiqiang scuec_weiqiang@qq.com
 * @LastEditTime: 2024-11-14 00:52:08
 * @FilePath: /my_code/source/page.c
 * @Description:
 * @
 * @Copyright (c) 2024 by  weiqiang scuec_weiqiang@qq.com , All Rights Reserved.
 ***************************************************************/
#include <os/malloc.h>
#include <asm/platform.h>
#include <os/printk.h>
#include <asm/spinlock.h>
#include <os/string.h>
#include <os/mm/symbols.h>
#include <os/types.h>
#include <os/page.h>
#include <os/mm/early_malloc.h>

struct spinlock page_lock = SPINLOCK_INIT;

static int is_init = 0;

// page management struct
typedef struct PageM {
    uint8_t flags;
} PageM_t;

#define PAGE_TOKEN 0x01
#define PAGE_LAST 0x02
#define _CLEAR(x) (x->flags = 0)
#define _IS_FREE(x) (!(x->flags & PAGE_TOKEN))
#define _IS_LAST(x) ((x->flags & PAGE_LAST) >> 1)
#define _SET_FLAG(x, y) (x->flags |= y)
#define _PAGE_IS_ALIGNED(addr) (((addr) & ((1 << PAGE_SHIFT) - 1)) == 0 ? 1 : 0)

static uintptr_t alloc_start = 0;
static uintptr_t alloc_end = 0;
static uint64_t pages_num = 0;

uint64_t remain_mem = RAM_SIZE;

void print_maddr() {
    printk("_text_start = %x---->", text_start);
    printk("_text_end = %x\n", text_end);
    printk("_rodata_start = %x---->", rodata_start);
    printk("_rodata_end = %x\n", rodata_end);
    printk("_data_start = %x---->", data_start);
    printk("_data_end = %x\n", data_end);
    printk("_bss_start = %x---->", bss_start);
    printk("_bss_end = %x\n", bss_end);
    printk("_heap_start = %x---->", heap_start);
    printk("_heap_end = %x\n", heap_end);
    printk("_heap_size = %x\n", heap_size);
    printk("_stack_start = %x---->", stack_start);
    printk("_stack_end = %x\n", stack_end);
}
/***************************************************************
 * @description:
 * @return {*}
 ***************************************************************/
void malloc_init() {
    /*
    保留 8*PAGE_SIZE 大小的内存用来管理page
    */
    pages_num = (heap_size - RESERVED_PAGE_SIZE) / PAGE_SIZE;
    alloc_start = heap_start + RESERVED_PAGE_SIZE;
    alloc_end = alloc_start + pages_num * PAGE_SIZE;
    printk("page init ... \n");
    printk("heap_start = %xu -----------------_heap_end = %x \n", heap_start, heap_end);
    printk("alloc_start = %xu\n", alloc_start);
    printk("alloc_end = %xu\n", alloc_end);
    printk("num_pages = %xu\n", pages_num);
    PageM_t *pagem_i = (PageM_t *)heap_start;
    for (int i = 0; i < pages_num; i++) {
        _CLEAR(pagem_i);
        pagem_i++;
    }
    remain_mem = pages_num * PAGE_SIZE;
    // printk("malloc init success\n");
}

/***************************************************************
 * @description:
 * @param {uint32_t} npages [in/out]:
 * @return {*}
 ***************************************************************/
void *page_alloc(size_t npages) {
    if (!is_init) {
        return early_page_alloc(npages);
    }
    spin_lock(&page_lock);
    uintptr_t reserved_end = (uintptr_t)heap_start + pages_num * sizeof(PageM_t);
    uint64_t num_blank = 0;
    PageM_t *pagem_i = (PageM_t *)heap_start;
    PageM_t *pagem_j = pagem_i;
    for (; (uint64_t)pagem_i < reserved_end; pagem_i++) {
        if (_IS_FREE(pagem_i)) // 如果是空白page
        {
            // 搜索此空白page以及后面page，是否连续空白page数满足分配要求
            for (pagem_j = pagem_i; ((uint64_t)pagem_j < reserved_end); pagem_j++) {
                if (_IS_FREE(pagem_j)) {
                    num_blank++;             // 对连续空白page计数
                    if (num_blank == npages) // 达到要求直接退出循环
                    {
                        break;
                    }
                } else {
                    num_blank = 0;
                    break;
                }
            }
            if (num_blank < npages) // 如果找不到足够数量的pages直接置零
            // 这样只要判断num_blank是否为0就知道能不能找到了
            {
                num_blank = 0;
            }
        }

        if (0 == num_blank) // 没找到接着后面继续找
        {
            pagem_i = pagem_j++;
        } else // 找到了，对pagem_i到pagem_j标志位置1，表明他们管理的内存被占用了
        {
            for (PageM_t *pagem_k = pagem_i; pagem_k < pagem_j; pagem_k++) {
                _SET_FLAG(pagem_k, PAGE_TOKEN);
            }
            _SET_FLAG(pagem_j, PAGE_TOKEN);
            _SET_FLAG(pagem_j, PAGE_LAST); // 表明它是末尾的内存page
            uintptr_t pgaddr =
                alloc_start + ((((uintptr_t)pagem_i - (uintptr_t)heap_start) / sizeof(PageM_t)) * PAGE_SIZE);
            // printk("pgaddr = %x\n",pgaddr);
            // memset((void*)pgaddr,0x00,npages*PAGE_SIZE);
            spin_unlock(&page_lock);
            remain_mem -= npages * PAGE_SIZE;
            return (void *)(pgaddr); // 找到直接返回
        }
    }
    spin_unlock(&page_lock);
    return NULL;
}
void print_page(uint64_t start, uint64_t end) {
    PageM_t *pagem_i = (PageM_t *)heap_start + start;
    for (int i = start; i < end; i++) {
        printk("pagem %x ->>%x = %x\n", pagem_i,
               alloc_start + ((((uintptr_t)pagem_i - (uintptr_t)heap_start) / sizeof(PageM_t)) * PAGE_SIZE),
               _IS_FREE(pagem_i));
        pagem_i++;
    }
}

/***************************************************************
 * @description:
 * @param {void*} p [in/out]:
 * @return {*}
 ***************************************************************/
void free(void *p) {
    if (!is_init) {
        return early_free(p);
    }

    spin_lock(&page_lock);
    if ((NULL == p)                                 // 传入的地址是空指针
        || ((uintptr_t)p > (alloc_end - PAGE_SIZE)) // 传入的地址在最后一个page之后
        || !(_PAGE_IS_ALIGNED((uintptr_t)p))        // 传入的地址不是4096对齐的
    ) {
        printk("free error\n");
        return;
    }

    PageM_t *pagem_i = (PageM_t *)(heap_start + ((((uintptr_t)p - alloc_start) / PAGE_SIZE) * sizeof(PageM_t)));

    for (; !_IS_LAST(pagem_i); pagem_i++) {
        remain_mem += PAGE_SIZE;
        _CLEAR(pagem_i);
    }
    remain_mem += PAGE_SIZE;
    _CLEAR(pagem_i);
    spin_unlock(&page_lock);
}

void *malloc(size_t size) {
    if (!is_init) {
        return early_malloc(size);
    }
    
    if (size == 0) {
        return NULL;
    }
    if (size > pages_num * PAGE_SIZE) {
        printk("malloc error: size too large\n");
        return NULL;
    }
    uint64_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE; // 向上取整
    void *p = page_alloc(npages);
    if (p == NULL) {
        printk("malloc error: no enough memory\n");
        return NULL;
    }
    return p;
}

#include <os/color.h>
uint64_t get_remain_mem() {
    printk(GREEN("remain mem = %d.%dMb\n"), remain_mem / 1024 / 1024, remain_mem / 1024 % 1024);
    return remain_mem;
}