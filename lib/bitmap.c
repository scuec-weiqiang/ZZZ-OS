/**
 * @FilePath: /ZZZ/lib/bitmap.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 17:54:37
 * @LastEditTime: 2025-06-30 11:18:54
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "bitmap.h"
#include "printf.h"
#include "string.h"
#include "page_alloc.h"

#define PAGE_SIZE 4096

typedef struct bitmap
{
    size_t size; // bitmap大小（位数）
    uint64_t arr[];   // bitmap数组
}bitmap_t;


bitmap_t* bitmap_create(size_t size)
{
    if(size==0 || size>UINT64_MAX/8)
    {
        printf("bitmap size error\n");
        return NULL;
    }

    uint64_t bytes_num = (size+7)/8;

    bitmap_t* bm = (bitmap_t *)malloc(sizeof(bitmap_t)+bytes_num);
    if(bm==NULL)
    {
        printf("bitmap: bitmap malloc error\n");
        return NULL;
    }

    bm->size = size;

    memset(bm->arr,0,bytes_num);

    return bm;
}


int64_t bitmap_destory(bitmap_t *bm)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return -1;
    }

    bm->size = 0;
    free(bm);

    return 0;
}


int64_t bitmap_set_bit(bitmap_t *bm, uint64_t index)
{

    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }

    if(index>=bm->size)
    {
        printf( ("bitmap: index out of range\n"));
        return -1;
    }

    uint64_t uint64_index = index / 64;
    uint64_t bit_index = index % 64;

    bm->arr[uint64_index] |= (1ULL << bit_index);

    return 0;   
}


int64_t bitmap_clear_bit(bitmap_t *bm, uint64_t index)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }
    if(index>=bm->size)
    {
        printf( ("bitmap: index out of range\n"));
        return -1;
    }

    uint64_t uint64_index = index / 64;
    uint64_t bit_index = index % 64;
    bm->arr[uint64_index] &= ~(1ULL << bit_index);
    return 0;   

}


int64_t bitmap_test_bit(bitmap_t *bm, uint64_t index)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }

    if(index>=bm->size)
    {
        printf( ("bitmap: index out of range\n"));
        return -1;
    }
    uint64_t uint64_index = index / 64;
    uint64_t bit_index = index % 64;
    return (bm->arr[uint64_index] & (1ULL << bit_index))==0?0:1;
}


size_t bitmap_get_size(bitmap_t *bm)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }
    return bm->size;
}


size_t bitmap_get_bytes_num(bitmap_t *bm)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }
    return (bm->size+7)/8;
}

size_t bitmap_get_size_in_bytes(bitmap_t *bm)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return 0;
    }
    return sizeof(bitmap_t)+bitmap_get_bytes_num(bm);
}

int64_t bitmap_scan_0(bitmap_t *bm)
{
    if(bm==NULL)
    {
        printf("bitmap: bitmap is not created\n");
        return -1;
    }

    for(uint64_t i=0;i<bm->size;i++)
    {
        if(bm->arr[i]!=UINT64_MAX)
        {
            uint8_t* arr=(uint8_t*)&bm->arr[i];
            for(uint8_t j=0;j<8;j++)
            {
                if(arr[j]!=UINT8_MAX)
                {
                    for(uint8_t k=0;k<8;k++)
                    {
                        if((arr[j]&(1<<k)) == 0)
                        {
                            return i*64+j*8+k;
                        }
                    }
                }
            }
        }
    }
    return -1;
}  

