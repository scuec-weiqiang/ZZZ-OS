/**
 * @FilePath: /ZZZ/drivers/virt_disk.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:44
 * @LastEditTime: 2025-05-28 16:32:06
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include "virtio.h"
#include "virt_disk.h"
#include "spinlock.h"
#include "string.h"
#include "buf.h"
#include "color.h"
#include "mm.h"
#include "vm.h"
#include "page_alloc.h"

typedef struct virt_disk
{
    virtq_t disk_queue;

    spinlock_t disk_lock;  
    // our own book-keeping.
    char free[QUEUE_NUM];  // is a descriptor free?
    uint16_t last_used_idx; // we've looked this far in used[2..NUM].

    // track info about in-flight operations,
    // for use when completion interrupt arrives.
    // indexed by first descriptor index of chain.
    // struct {
    //     struct buf *b;
    //     char status;
    // } info[QUEUE_NUM];

    // disk command headers.
    // one-for-one with descriptors, for convenience.
    struct virtio_blk_req req[QUEUE_NUM];
    uint8_t status[QUEUE_NUM];
}virt_disk_t;

virt_disk_t virt_disk;

void virt_disk_init(void)
{
    virtio_blk_init();

    virtio->queue_sel = VIRT_DISK_QUEUE;

    if(virtio->queue_num_max == 0)
    {
        panic(RED("virtio disk has no queue!\n"));
    }
    else if(virtio->queue_num_max < QUEUE_NUM)
    {
        panic(RED("virtio disk has too few queues!\n"));
    }

    virt_disk.disk_queue.desc = (virtq_desc_t*)page_alloc(1);
    virt_disk.disk_queue.avail = (virtq_avail_t*)page_alloc(1);
    virt_disk.disk_queue.used = (virtq_used_t*)page_alloc(1);

    memset(virt_disk.disk_queue.desc, 0, 4096);
    memset(virt_disk.disk_queue.avail, 0, 4096);
    memset(virt_disk.disk_queue.used, 0, 4096);

    virtio->queue_num = QUEUE_NUM;

    virtio->queue_desc_low = (uint32_t)((uint64_t) virt_disk.disk_queue.desc & 0xFFFFFFFF);
    virtio->queue_desc_high = (uint32_t)((uint64_t)virt_disk.disk_queue.desc >> 32);

    virtio->queue_avail_low = (uint32_t)((uint64_t)virt_disk.disk_queue.avail & 0xFFFFFFFF);
    virtio->queue_avail_high = (uint32_t)((uint64_t)virt_disk.disk_queue.avail >> 32);

    virtio->queue_used_low= (uint32_t)((uint64_t)virt_disk.disk_queue.used & 0xFFFFFFFF);
    virtio->queue_used_high = (uint32_t)((uint64_t)virt_disk.disk_queue.used >> 32);


    virtio->queue_ready = 1;

    if(!virtio->queue_ready)
    {
        panic(RED("virtio disk is not ready!\n"));
    }

    __sync_synchronize();


    for(uint32_t i = 0;i < QUEUE_NUM;i++)
    {
        virt_disk.free[i] = 1;// 1表示空闲，0表示正在使用
    }

    virtio->status |= VIRTIO_CONFIG_S_DRIVER_OK;

}

__SELF int alloc_disk_desc(void)
{
    for (uint32_t i = 0; i < QUEUE_NUM; i++)
    {
        if (virt_disk.free[i]==1) //找一个没被使用的描述符
        {
            virt_disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

__SELF void free_disk_desc(int index)
{
    if (index >= 0 && index < QUEUE_NUM)
    {
        virt_disk.free[index] = 1;

        virt_disk.disk_queue.desc[index].addr = 0;
        virt_disk.disk_queue.desc[index].len = 0;
        virt_disk.disk_queue.desc[index].flags = 0;
        virt_disk.disk_queue.desc[index].next = 0;
    }
    else
    {
        printf(YELLOW("free_disk_desc: index out of range!\n"));
    }
}

/**
 * @brief 释放磁盘链中的描述符
 *
 * 释放指定索引位置的磁盘描述符，并继续释放其链中的下一个描述符，直到没有更多的描述符为止。
 *
 * @param index 要释放的第一个描述符的索引
 */
__SELF void free_disk_chain(int index)
{
    while (1)
    {
        free_disk_desc(index);
        uint16_t next = virt_disk.disk_queue.desc[index].next;
        if(virt_disk.disk_queue.desc[index].flags & VIRTQ_DESC_F_NEXT)
        {
            index = next;
        }
        else
        {
            break;
        }
    }
}


int virt_disk_rw(void *buffer, uint64_t sector, virt_disk_rw_t rwflag)
{
    spin_lock(&virt_disk.disk_lock);

    // 一次读写需要3个描述符，分别是：
    // 1. 请求头
    // 2. 数据缓冲区
    // 3. 状态码
    int index[3];
    for(int i = 0;i < 3;i++)
    {
        index[i] = alloc_disk_desc();
        if (index[i] < 0)
        {
            printf("alloc_disk_desc failed\n");
            return;
        }
    }

    //配置请求头
    if(rwflag == VIRT_DISK_READ) // 从磁盘读取数据
    {
        virt_disk.req[index[0]].type = VIRTIO_BLK_T_IN; 
    }
    else // 向磁盘写入数据
    {
        virt_disk.req[index[0]].type = VIRTIO_BLK_T_OUT; // 写操作
    }
    virt_disk.req[index[0]].reserved = 0;
    virt_disk.req[index[0]].sector = sector; // 指明向哪一个扇区读写

    // 讲请求头与数据缓冲区绑定起来 
    virt_disk.disk_queue.desc[index[0]].addr = (uint64_t) &virt_disk.req[index[0]];
    virt_disk.disk_queue.desc[index[0]].len = sizeof(struct virtio_blk_req);
    virt_disk.disk_queue.desc[index[0]].flags = VIRTQ_DESC_F_NEXT;
    virt_disk.disk_queue.desc[index[0]].next = index[1];

    // 配置数据缓冲区
    virt_disk.disk_queue.desc[index[1]].addr = (uint64_t)buffer;
    virt_disk.disk_queue.desc[index[1]].len = BLOCK_SIZE;
    if (rwflag == VIRT_DISK_READ)// 如果是从磁盘读数据 
    {
        virt_disk.disk_queue.desc[index[1]].flags = VIRTQ_DESC_F_NEXT|VIRTQ_DESC_F_WRITE; // 那就要告诉设备，表明设备需要往这里写数据
    }
    else // 如果是向磁盘写数据
    {
        virt_disk.disk_queue.desc[index[1]].flags =  VIRTQ_DESC_F_NEXT|0; // 则不需要指明
    }
    virt_disk.disk_queue.desc[index[1]].next = index[2];

    // 配置状态码
    virt_disk.disk_queue.desc[index[2]].addr = (uint64_t)&virt_disk.status[index[0]];
    virt_disk.disk_queue.desc[index[2]].len = 1;
    virt_disk.disk_queue.desc[index[2]].flags = VIRTQ_DESC_F_WRITE;
    virt_disk.disk_queue.desc[index[2]].next = 0;

    
    virt_disk.disk_queue.avail->ring[virt_disk.disk_queue.avail->idx % QUEUE_NUM] = index[0];
    __sync_synchronize();
    virt_disk.disk_queue.avail->idx += 1;
    __sync_synchronize();

    // 通知设备处理队列0中的请求
    virtio->queue_notify = 0;

    while(virt_disk.last_used_idx == virt_disk.disk_queue.used->idx){}
    virt_disk.last_used_idx = virt_disk.disk_queue.used->idx;
   
    spin_unlock(&virt_disk.disk_lock);
    if (virt_disk.status[index[0]] != 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }

}
