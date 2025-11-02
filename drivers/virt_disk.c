/**
 * @FilePath: /ZZZ-OS/drivers/virt_disk.c
 * @Description:  
 * @Author: scuec_weiqiang scuec_weiqiang@qq.com
 * @Date: 2025-05-23 15:56:44
 * @LastEditTime: 2025-10-31 01:01:34
 * @LastEditors: scuec_weiqiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2025.
*/
#include <os/types.h>
#include <drivers/virtio.h>
#include <drivers/virt_disk.h>
#include <os/string.h>
#include <fs/block_device.h>
#include <os/printk.h>
#include <os/malloc.h>
#include <asm/pgtbl.h>

struct virt_disk
{
    struct virtq disk_queue;
    char free[QUEUE_NUM];  
    int last_used_idx; 
    struct virtio_blk_req req[QUEUE_NUM];
    char status[QUEUE_NUM];

    struct disk_info{
        int capacity;
        int blk_size;
        int blk_num;
    } disk_info;
};

struct virt_disk virt_disk;

char desc[] = {0};
char avail[sizeof(struct virtq_avail)] = {0};
char used[sizeof(struct virtq_used)] = {0};

static int alloc_disk_desc(void)
{
    for (int i = 0; i < QUEUE_NUM; i++)
    {
        if (virt_disk.free[i]==1) //找一个没被使用的描述符
        {
            virt_disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

static void free_disk_desc(int index)
{
    if (index >= 0 && index < QUEUE_NUM)
    {
        virt_disk.free[index] = 1;

        // virt_disk.disk_queue.desc[index].addr = 0;
        // virt_disk.disk_queue.desc[index].len = 0;
        // virt_disk.disk_queue.desc[index].flags = 0;
        // virt_disk.disk_queue.desc[index].next = 0;
    }
    else
    {
        printk("free_disk_desc: index out of range!\n");
    }
}

/**
 * @brief 释放磁盘链中的描述符
 *
 * 释放指定索引位置的磁盘描述符，并继续释放其链中的下一个描述符，直到没有更多的描述符为止。
 *
 * @param index 要释放的第一个描述符的索引
 */
static void free_disk_chain(int index)
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

/**
 * @brief 对虚拟磁盘进行读写操作
 *
 * 该函数用于对虚拟磁盘进行读写操作。它首先获取自旋锁，然后分配三个描述符，分别用于请求头、数据缓冲区和状态码。
 * 根据读写标志（rwflag）配置请求头，指明是读取数据还是写入数据，并设置扇区号。接着，它将请求头与数据缓冲区绑定起来，
 * 并根据读写操作配置数据缓冲区的标志。然后，它配置状态码的描述符，并将请求添加到可用队列中。最后，它通知设备处理队列中的请求，
 * 等待请求处理完成，释放描述符链，并释放自旋锁。如果操作成功，返回0；否则，返回-1。
 *
 * @param buffer 数据缓冲区指针,大小为SECTOR_SIZE
 * @param sector 扇区号
 * @param rwflag 读写标志（VIRT_DISK_READ为读取，VIRT_DISK_WRITE为写入）
 * @return 操作成功返回0，失败返回-1
 */
int virt_disk_rw(void *buffer, int sector, enum virt_disk_rw rwflag)
{
    // 一次读写需要3个描述符，分别是：
    // 1. 请求头
    // 2. 数据缓冲区
    // 3. 状态码
    int index[3];
    for(int i = 0;i < 3;i++)
    {
        index[i] = alloc_disk_desc();
        if(index[i] < 0) return -1;
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
    virt_disk.disk_queue.desc[index[0]].addr = KERNEL_PA((uint64_t)&virt_disk.req[index[0]]);
    virt_disk.disk_queue.desc[index[0]].len = sizeof(struct virtio_blk_req);
    virt_disk.disk_queue.desc[index[0]].flags = VIRTQ_DESC_F_NEXT;
    virt_disk.disk_queue.desc[index[0]].next = index[1];

    // 配置数据缓冲区
    virt_disk.disk_queue.desc[index[1]].addr = KERNEL_PA((uint64_t)buffer);
    virt_disk.disk_queue.desc[index[1]].len = SECTOR_SIZE;
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
    virt_disk.disk_queue.desc[index[2]].addr = KERNEL_PA((uint64_t)&virt_disk.status[index[0]]);
    virt_disk.disk_queue.desc[index[2]].len = 1;
    virt_disk.disk_queue.desc[index[2]].flags = VIRTQ_DESC_F_WRITE;
    virt_disk.disk_queue.desc[index[2]].next = 0;

    
    virt_disk.disk_queue.avail->ring[virt_disk.disk_queue.avail->idx % QUEUE_NUM] = index[0];
    __sync_synchronize();
    virt_disk.disk_queue.avail->idx += 1;
    __sync_synchronize();

    // 通知设备处理队列0中的请求
    virtio->queue_notify = 0;

    while(virt_disk.last_used_idx == virt_disk.disk_queue.used->idx){
        // printk(YELLOW("virt_disk_rw: waiting for disk operation to complete..."));
    }
    virt_disk.last_used_idx = virt_disk.disk_queue.used->idx;
    free_disk_chain(index[0]);
    if (virt_disk.status[index[0]] != 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }

}

int virt_disk_get_capacity(void)
{
    if(virt_disk.disk_info.capacity == 0)
    {
        return -1;
    }
    return virt_disk.disk_info.capacity;
}


int disk_write_sector(void *buffer, int sector)
{
    return virt_disk_rw(buffer, sector, VIRT_DISK_WRITE);
}

int disk_read_sector(void *buffer, int sector)
{
    return virt_disk_rw(buffer, sector, VIRT_DISK_READ);
}

int virt_disk_init()
{

    virtio_blk_init();

    virtio->queue_sel = VIRT_DISK_QUEUE;

    if(virtio->queue_num_max == 0)
    {
        return -1;
    }
    else if(virtio->queue_num_max < QUEUE_NUM)
    {
        return -1;
    }

    virt_disk.disk_queue.desc = (struct virtq_desc*)page_alloc(1);
    virt_disk.disk_queue.avail = (struct virtq_avail*)page_alloc(1);
    virt_disk.disk_queue.used = (struct virtq_used*)page_alloc(1);

    virtio->queue_num = QUEUE_NUM;

    virtio->queue_desc_low = (uint32_t)((uintptr_t) KERNEL_PA(virt_disk.disk_queue.desc) & 0xFFFFFFFF);
    virtio->queue_desc_high = (uint32_t)((uintptr_t)KERNEL_PA(virt_disk.disk_queue.desc) >> 32);

    virtio->queue_avail_low = (uint32_t)((uintptr_t)KERNEL_PA(virt_disk.disk_queue.avail) & 0xFFFFFFFF);
    virtio->queue_avail_high = (uint32_t)((uintptr_t)KERNEL_PA(virt_disk.disk_queue.avail) >> 32);

    virtio->queue_used_low= (uint32_t)((uintptr_t)KERNEL_PA(virt_disk.disk_queue.used) & 0xFFFFFFFF);
    virtio->queue_used_high = (uint32_t)((uintptr_t)KERNEL_PA(virt_disk.disk_queue.used) >> 32);

    virtio->queue_ready = 1;

    if(!virtio->queue_ready)
    {
        return -1;
    }

    virtio->status |= VIRTIO_CONFIG_S_DRIVER_OK;

    __sync_synchronize();

    for(uint32_t i = 0;i < QUEUE_NUM;i++)
    {
        virt_disk.free[i] = 1;// 1表示空闲，0表示正在使用
    }

    struct virtio_blk_config* info = (struct virtio_blk_config*)&virtio->config[0];
    virt_disk.disk_info.blk_size = info->blk_size;
    virt_disk.disk_info.blk_num = info->capacity;//从硬件中读取的capacity是以块为单位，
    virt_disk.disk_info.capacity = info->capacity*info->blk_size; // 所以要乘以每个块的字节数
    
    static struct block_device virt_bdev;

    strcpy(virt_bdev.name,"virt_disk");
    virt_bdev.private_data = (void*)&virt_disk;
    virt_bdev.sector_size = 512;         // 设备块大小
    virt_bdev.capacity = info->capacity*512;// 返回总块数
    virt_bdev.read = disk_read_sector;
    virt_bdev.write = disk_write_sector;

    // 注册设备
    if(block_device_register(&virt_bdev) < 0)
    {
        return -1;
    }
    return 0;
}