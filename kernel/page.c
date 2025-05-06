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
#include "printf.h"
#include "types.h"

#include "maddr_def.h"

//page management struct
typedef struct PageM
{
    uint8_t flags;
}PageM_t;

#define PAGE_SIZE                   4096
#define NUM_RESERVED_PAGES          8
#define RESERVED_PAGE_SIZE          NUM_RESERVED_PAGES*PAGE_SIZE 
#define PAGE_ORDER                  12

#define PAGE_TOKEN              0x01
#define PAGE_LAST               0x02
#define _CLEAR(x)               (x->flags = 0)
#define _IS_FREE(x)             (!(x->flags&PAGE_TOKEN))
#define _IS_LAST(x)             ((x->flags&PAGE_LAST)>>1)
#define _SET_FLAG(x,y)          (x->flags|=y)
#define _PAGE_IS_ALIGNED(addr)  (((addr) & ((1<<PAGE_ORDER) - 1))==0?1:0)

static addr_t _alloc_start = 0;
static addr_t  _alloc_end = 0;
static uint64_t _num_pages = 0;

/***************************************************************
 * @description: 
 * @return {*}
***************************************************************/
void page_init()
{
    /*
    保留 8*PAGE_SIZE 大小的内存用来管理page
    */
    _num_pages = ((addr_t)_heap_size-RESERVED_PAGE_SIZE)/PAGE_SIZE;
    _alloc_start = (addr_t)_heap_start + RESERVED_PAGE_SIZE; 
    _alloc_end = _alloc_start + _num_pages*PAGE_SIZE;
    printf("page init ... \n");
    printf("_heap_start = %x -----------------_heap_end = %x \n",_heap_start,_heap_end);
    printf("_alloc_start = %x\n",_alloc_start);
    printf("_alloc_end = %x\n",_alloc_end);
    printf("_num_pages = %x\n",_num_pages);
    PageM_t *pagem_i = (PageM_t*)_heap_start;
    for(int i=0;i<_num_pages;i++)
    {
        _CLEAR(pagem_i);
        pagem_i++;
    }
    printf("page init success\n");
}


/***************************************************************
 * @description: 
 * @param {uint32_t} npages [in/out]:  
 * @return {*}
***************************************************************/
void* page_alloc(uint64_t npages)
{   
    addr_t reserved_end = (addr_t)_heap_start + _num_pages*sizeof(PageM_t);
    uint64_t num_blank = 0;
    PageM_t *pagem_i = (PageM_t*)_heap_start;
    PageM_t *pagem_j = pagem_i;
    for(;(uint64_t)pagem_i < reserved_end; pagem_i++)
    {
        if(_IS_FREE(pagem_i))//如果是空白page
        {   
            //搜索此空白page以及后面page，是否连续空白page数满足分配要求
            for(pagem_j = pagem_i;((uint64_t)pagem_j < reserved_end);pagem_j++)
            {
                if(_IS_FREE(pagem_j))
                {
                    num_blank++;//对连续空白page计数
                    if(num_blank == npages)//达到要求直接退出循环
                    {
                        break;
                    }
                }
                else
                {
                    num_blank = 0;
                    break;
                }

            }
            if(num_blank < npages)//如果找不到足够数量的pages直接置零
            //这样只要判断num_blank是否为0就知道能不能找到了
            {
                num_blank = 0;
            }

        }
        if(0 == num_blank)//没找到接着后面继续找
        {
            pagem_i  = pagem_j++;
        }
        else//找到了，对pagem_i到pagem_j标志位置1，表明他们管理的内存被占用了
        {
            for(PageM_t *pagem_k = pagem_i;pagem_k<pagem_j;pagem_k++)
            {
                _SET_FLAG(pagem_k,PAGE_TOKEN);
            }
            _SET_FLAG(pagem_j,PAGE_TOKEN);
            _SET_FLAG(pagem_j,PAGE_LAST);//表明它是末尾的内存page
            return (void*)(_alloc_start + ((((addr_t)pagem_i - (addr_t)_heap_start)/sizeof(PageM_t))*PAGE_SIZE));//找到直接返回
        }
        
    }
    return NULL_PTR;
}


/***************************************************************
 * @description: 
 * @param {void*} p [in/out]:  
 * @return {*}
***************************************************************/
void page_free(void* p)
{  
    if((NULL_PTR == p)//传入的地址是空指针
        ||((addr_t)p > (_alloc_end-PAGE_SIZE))//传入的地址在最后一个page之后
        ||(_PAGE_IS_ALIGNED((addr_t)p))//传入的地址不是4096对齐的
        )
    {
        return;
    }

    PageM_t *pagem_i = (PageM_t *)(_heap_start + ((((addr_t)p-_alloc_start)/PAGE_SIZE)*sizeof(PageM_t)));

    for(;!_IS_LAST(pagem_i);pagem_i++)
    {
        _CLEAR(pagem_i);
    }
    _CLEAR(pagem_i);
}