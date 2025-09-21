/**
 * @FilePath: /ZZZ/lib/bitmap.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-30 17:54:37
 * @LastEditTime: 2025-09-20 16:58:05
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "bitmap.h"
#include "printk.h"
#include "string.h"
#include "malloc.h"


struct bitmap
{
    size_t size; // bitmap大小（位数）
    u64 arr[];   // bitmap数组
};


struct bitmap* bitmap_create(size_t size)
{
    if(size==0 || size>U64_MAX/8)
    {
        printk("bitmap size error\n");
        return NULL;
    }

    u64 bytes_num = (size+7)/8;

    struct bitmap* bm = (struct bitmap *)malloc(sizeof(struct bitmap)+bytes_num);
    if(bm==NULL)
    {
        printk("bitmap: bitmap malloc error\n");
        return NULL;
    }

    bm->size = size;

    memset(bm->arr,0,bytes_num);

    return bm;
}


void bitmap_destory(struct bitmap *bm)
{
    free(bm);
}

int bitmap_set_bit(struct bitmap *bm, u64 index)
{

    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }

    if(index>=bm->size)
    {
        printk( ("bitmap: index out of range\n"));
        return -1;
    }

    u64 uint64_index = index / 64;
    u64 bit_index = index % 64;

    bm->arr[uint64_index] |= (1ULL << bit_index);

    return 0;   
}


int bitmap_clear_bit(struct bitmap *bm, u64 index)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }
    if(index>=bm->size)
    {
        printk( ("bitmap: index out of range\n"));
        return -1;
    }

    u64 uint64_index = index / 64;
    u64 bit_index = index % 64;
    bm->arr[uint64_index] &= ~(1ULL << bit_index);
    return 0;   

}

/**
* @brief 测试位图中的某个位是否为1
*
* 根据给定的索引位置，检查位图中对应的位是否为1。
*
* @param bm 位图对象指针
* @param index 需要测试的位索引位置
*
* @return 如果位为1，则返回1；如果位为0，则返回0；如果位图未创建或索引超出范围，则返回相应错误码
*         - 如果位图为空（bm为NULL），返回0，并打印错误信息 "bitmap: bitmap is not created"
*         - 如果索引超出位图范围，返回-1，并打印错误信息 "bitmap: index out of range"
*/
int bitmap_test_bit(struct bitmap *bm, u64 index)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }

    if(index>=bm->size)
    {
        printk( ("bitmap: index out of range\n"));
        return -1;
    }
    u64 uint64_index = index / 64;
    u64 bit_index = index % 64;
    return (bm->arr[uint64_index] & (1ULL << bit_index))==0?0:1;
}

size_t bitmap_get_size(struct bitmap *bm)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }
    return bm->size;
}

size_t bitmap_update_size(struct bitmap *bm,u64 size)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }

    bm->size = size;
    return bm->size;
}

size_t bitmap_get_bytes_num(struct bitmap *bm)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }
    return (bm->size+7)/8;
}

size_t bitmap_get_size_in_bytes(struct bitmap *bm)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return 0;
    }
    return sizeof(struct bitmap)+bitmap_get_bytes_num(bm);
}

int bitmap_scan_0(struct bitmap *bm)
{
    if(bm==NULL)
    {
        printk("bitmap: bitmap is not created\n");
        return -1;
    }

    for(u64 i=0;i<bm->size;i++)
    {
        if(bm->arr[i]!=U64_MAX)
        {
            char* arr=(char*)&bm->arr[i];
            for(int j=0;j<8;j++)
            {
                if(arr[j]!=U8_MAX)
                {
                    for(int k=0;k<8;k++)
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

