#include <mm/buddy.h>
#include <os/errno.h>
#include <os/fb.h>
#include <os/init.h>
#include <os/kmalloc.h>
#include <os/kva.h>
#include <os/pfn.h>
#include <os/printk.h>
#include <os/qemu_fw_cfg.h>
#include <os/string.h>

#define SIMPLEFB_WIDTH  640U
#define SIMPLEFB_HEIGHT 400U
#define SIMPLEFB_BPP    32U

static struct fb_info simplefb_info;
static size_t simplefb_npages;

static void simplefb_draw_color_bars(struct fb_info *info)
{
    static const u32 colors[] = {
        0x00ffffff, /* white */
        0x00ffff00, /* yellow */
        0x0000ffff, /* cyan */
        0x0000ff00, /* green */
        0x00ff00ff, /* magenta */
        0x00ff0000, /* red */
        0x000000ff, /* blue */
        0x00000000, /* black */
    };
    u32 *pixels = info->screen_base;
    u32 x;
    u32 y;

    for (y = 0; y < info->height; y++) {
        for (x = 0; x < info->width; x++) {
            size_t index = (size_t)y * info->width + x;
            unsigned int bar = x *
                (sizeof(colors) / sizeof(colors[0])) / info->width;

            pixels[index] = colors[bar];
        }
    }
}

static int simplefb_init(void)
{
    struct fb_info *info = &simplefb_info;
    int ret;

    memset(info, 0, sizeof(*info));
    info->width = SIMPLEFB_WIDTH;
    info->height = SIMPLEFB_HEIGHT;
    info->bpp = SIMPLEFB_BPP;
    info->pitch = info->width * (info->bpp / 8U);
    info->size = (size_t)info->pitch * info->height;

    simplefb_npages = PAGE_ALIGN(info->size) / PAGE_SIZE;
    info->screen_base = page_alloc(simplefb_npages);
    if (!info->screen_base)
        return -ENOMEM;

    memset(info->screen_base, 0, simplefb_npages * PAGE_SIZE);
    info->screen_phys = KERNEL_PA(info->screen_base);
    simplefb_draw_color_bars(info);

    ret = register_framebuffer(info, NULL);
    if (ret) {
        free_pages_kva(info->screen_base);
        info->screen_base = NULL;
        return ret;
    }

    ret = qemu_ramfb_configure(info);
    if (ret)
        printk("simplefb: qemu ramfb configure failed: %d\n", ret);

    printk("simplefb: memory=%lx phys=%lx size=%lu pages=%lu\n",
           (unsigned long)info->screen_base,
           (unsigned long)info->screen_phys,
           (unsigned long)info->size,
           (unsigned long)simplefb_npages);
    return 0;
}

static void simplefb_exit(void)
{
    struct fb_info *info = &simplefb_info;

    unregister_framebuffer(info);
    if (info->screen_base) {
        free_pages_kva(info->screen_base);
        info->screen_base = NULL;
    }
}

device_initcall(simplefb_init);
module_exit(simplefb_exit, ".exitcall");
