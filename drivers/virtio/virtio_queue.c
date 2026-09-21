/**
 * @FilePath     : /ZZZ-OS/drivers/virtio/virtio_queue.c
 * @Description  :  
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-09-16 16:25:00
 * @LastEditTime : 2026-09-16 19:49:15
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
*/

#include <os/virtio.h>
#include <os/kmalloc.h>
#include <os/string.h>
#include <os/err.h>
#include <os/spinlock.h>
#include <os/kva.h>

#define VIRTQUEUE_DEFAULT_SIZE 128
#define VIRTQUEUE_END          0xffff

// virtio 设备初始化时申请一个virtqueue，分配描述符、avail、used三个环形缓冲区，并初始化队列状态
struct virtqueue *virtqueue_create(struct virtio_device *vdev, 
                                unsigned int index, 
                                unsigned int requested_num,
                                void (*callback)(struct virtqueue *vq))   
{
    if (requested_num == 0 || requested_num > VIRTQUEUE_DEFAULT_SIZE)
        requested_num = VIRTQUEUE_DEFAULT_SIZE;

    struct virtqueue *vq = kzalloc(sizeof(struct virtqueue));
    if (!vq)
        return NULL;

    vq->vdev = vdev;
    vq->index = index;
    vq->num = requested_num;
    vq->callback = callback;
    spin_lock_init(&vq->lock);

    vq->desc = page_alloc(1);
    if (!vq->desc)
        goto failed;
    vq->avail = page_alloc(1);
    if (!vq->avail)
        goto failed;
    vq->used = page_alloc(1);
    if (!vq->used)
        goto failed;

    memset(vq->desc, 0, PAGE_SIZE);
    memset(vq->avail, 0, PAGE_SIZE);
    memset(vq->used, 0, PAGE_SIZE);

    vq->tokens = kzalloc(sizeof(void *) * vq->num);
    if (!vq->tokens)
        goto failed;

    // 这里可能将vq->num调整为设备支持的最大队列长度
    int ret = vdev->config->setup_vq(vdev, vq, index);
    if (ret)
        goto failed;

    for(int i = 0; i < vq->num; i++) {
        vq->desc[i].next = i + 1;
    }
    vq->desc[vq->num - 1].next = VIRTQUEUE_END;

    vq->free_head = 0;
    vq->num_free = vq->num;
    vq->avail_idx = 0;
    vq->last_used_idx = 0;

    list_add_tail(&vdev->vqs, &vq->node);
    return vq;

failed:
    kfree(vq->tokens);
    if (vq->used)
        page_free(vq->used);
    if (vq->avail)
        page_free(vq->avail);
    if (vq->desc)
        page_free(vq->desc);
    kfree(vq);
    return NULL;
}

int virtqueue_add(struct virtqueue *vq,
                  struct virtio_buffer *bufs,
                  unsigned int num_bufs,
                  void *token)
{
    if (!vq || !bufs || num_bufs == 0)
        return -EINVAL;

    unsigned long irq_flags;
    spin_lock_irqsave(&vq->lock, &irq_flags);

    if (vq->num_free < num_bufs) {
        spin_unlock_irqrestore(&vq->lock, irq_flags);
        return -ENOSPC;
    }

    u16 this_head = vq->free_head;
    for (int i = 0; i < num_bufs; i++) {
        u16 current = vq->free_head;
        struct virtio_buffer *buf = &bufs[i];
        vq->free_head = vq->desc[current].next;

        vq->desc[current].addr = (u64)KERNEL_PA(buf->addr);
        vq->desc[current].len = buf->len;
        vq->desc[current].flags = buf->device_write ? VIRTQ_DESC_F_WRITE : 0;

        if (i < num_bufs - 1) {
            vq->desc[current].flags |= VIRTQ_DESC_F_NEXT;
            vq->desc[current].next = vq->free_head;
        } else {
            vq->desc[current].next = VIRTQUEUE_END;
        }
        vq->num_free--;
    }
    vq->tokens[this_head] = token;
    
    wmb(); // 确保描述符写入内存后再更新avail ring
    vq->avail->ring[vq->avail_idx % vq->num] = this_head;
    wmb(); // 确保avail ring写入内存后再更新avail_idx
    vq->avail_idx++;
    vq->avail->idx = vq->avail_idx;

    spin_unlock_irqrestore(&vq->lock, irq_flags);
    return 0;
}


void virtqueue_kick(struct virtqueue *vq)
{
    if (!vq)
        return;

    wmb();

    vq->vdev->config->notify(vq);
}

void *virtqueue_get_buf(struct virtqueue *vq, unsigned int *len)
{
    if (!vq || !len)
        return NULL;

    unsigned long irq_flags;
    spin_lock_irqsave(&vq->lock, &irq_flags);

    if (vq->last_used_idx == vq->used->idx) {
        spin_unlock_irqrestore(&vq->lock, irq_flags);
        return NULL;
    }

    rmb(); // 确保读取used ring前，先读取used->idx

    u32 used_idx = vq->last_used_idx % vq->num;
    struct virtq_used_elem *used_elem = &vq->used->ring[used_idx];
    u32 head = used_elem->id;
    if (head >= vq->num) {
        spin_unlock_irqrestore(&vq->lock, irq_flags);
        return NULL;
    }
    *len = used_elem->len;

    void *token = vq->tokens[head];
    vq->tokens[head] = NULL;


    // 回收整条 descriptor chain，包括最后一个 descriptor
    for (;;) {
        u16 next = vq->desc[head].next;
        bool has_next = vq->desc[head].flags & VIRTQ_DESC_F_NEXT;

        vq->desc[head].flags = 0;
        vq->desc[head].next = vq->free_head;
        vq->free_head = head;
        vq->num_free++;

        if (!has_next)
            break;
        head = next;
    }

    vq->last_used_idx++;
    
    spin_unlock_irqrestore(&vq->lock, irq_flags);
    return token;
}
