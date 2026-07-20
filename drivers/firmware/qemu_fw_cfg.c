/**
 * @FilePath     : /ZZZ-OS/drivers/firmware/qemu_fw_cfg.c
 * @Description  :
 * @Author       : WeiQiang scuec_weiqiang@qq.com
 * @Date         : 2026-07-17 16:19:24
 * @LastEditTime : 2026-07-18 01:35:18
 * @LastEditors  : WeiQiang scuec_weiqiang@qq.com
 * @Copyright    : G AUTOMOBILE RESEARCH INSTITUTE CO.,LTD Copyright (c) 2026.
 */
#include <os/bswap.h>
#include <os/errno.h>
#include <os/fb.h>
#include <os/init.h>
#include <os/io.h>
#include <os/kva.h>
#include <os/mm.h>
#include <os/platform_device.h>
#include <os/printk.h>
#include <os/qemu_fw_cfg.h>
#include <os/string.h>
#include <uapi/drm/drm_fourcc.h>

#define FW_CFG_SIGNATURE 0x0000
#define FW_CFG_MMIO_SIZE 0x18
#define FW_CFG_REG_DATA 0x00
#define FW_CFG_REG_SEL 0x08
#define FW_CFG_REG_DMA 0x10
#define FW_CFG_DMA_POLL_LIMIT 1000000U

struct qemu_fw_cfg {
    virt_addr_t base;
};

static struct qemu_fw_cfg fw_cfg;

static inline __be16 cpu_to_be16(u16 x) {
    return (x << 8) | (x >> 8);
}

static inline u16 be16_to_cpu(__be16 x) {
    return cpu_to_be16(x);
}

static inline void fw_cfg_select(u16 selector) {
    writew(cpu_to_be16(selector), fw_cfg.base + FW_CFG_REG_SEL);
}

static inline u8 fw_cfg_read_byte(void) {
    return readb(fw_cfg.base + FW_CFG_REG_DATA);
}

static void fw_cfg_read(void *buf, size_t size) {
    u8 *p = buf;
    size_t i;

    for (i = 0; i < size; i++)
        p[i] = fw_cfg_read_byte();
}

bool qemu_fw_cfg_ready(void) {
    return fw_cfg.base != 0;
}

int qemu_fw_cfg_init(phys_addr_t base) {
    char sig[4];

    fw_cfg.base = (virt_addr_t)ioremap(base, FW_CFG_MMIO_SIZE);
    if (!fw_cfg.base)
        return -ENOMEM;

    fw_cfg_select(FW_CFG_SIGNATURE);
    fw_cfg_read(sig, sizeof(sig));
    if (memcmp(sig, "QEMU", sizeof(sig)) != 0) {
        printk("qemu_fw_cfg: bad signature %c%c%c%c\n", sig[0], sig[1], sig[2], sig[3]);
        iounmap(fw_cfg.base, FW_CFG_MMIO_SIZE);
        fw_cfg.base = 0;
        return -ENODEV;
    }

    printk("qemu_fw_cfg: mmio base=%lx\n", (unsigned long)fw_cfg.base);
    return 0;
}

int qemu_fw_cfg_find_file(const char *name, u16 *selector, u32 *size) {
    __be32 be_count;
    u32 count;
    u32 i;

    if (!qemu_fw_cfg_ready())
        return -ENODEV;

    fw_cfg_select(FW_CFG_FILE_DIR);
    fw_cfg_read(&be_count, sizeof(be_count));
    count = be32_to_cpu(be_count);

    for (i = 0; i < count; i++) {
        struct fw_cfg_file file;

        fw_cfg_read(&file, sizeof(file));
        if (strcmp(file.name, name) == 0) {
            if (selector)
                *selector = be16_to_cpu(file.select);
            if (size)
                *size = be32_to_cpu(file.size);
            return 0;
        }
    }

    return -ENOENT;
}

int qemu_fw_cfg_write_file(const char *name, const void *data, size_t size) {
    u16 selector;
    u32 file_size;
    int ret;

    ret = qemu_fw_cfg_find_file(name, &selector, &file_size);
    if (ret)
        return ret;
    if (size > file_size)
        return -EINVAL;

    return qemu_fw_cfg_dma_write(selector, data, size);
}

int qemu_fw_cfg_dma_write(u16 selector, const void *data, size_t size) {
    volatile struct fw_cfg_dma_access access;
    u32 control;
    unsigned int i;

    if (!qemu_fw_cfg_ready())
        return -ENODEV;
    if (!data || !size)
        return -EINVAL;

    control = FW_CFG_DMA_CTL_SELECT | FW_CFG_DMA_CTL_WRITE | ((u32)selector << 16);
    access.control = cpu_to_be32(control);
    access.length = cpu_to_be32(size);
    access.address = cpu_to_be64(KERNEL_PA(data));

    writeq(cpu_to_be64(KERNEL_PA(&access)), fw_cfg.base + FW_CFG_REG_DMA);

    for (i = 0; i < FW_CFG_DMA_POLL_LIMIT; i++) {
        control = be32_to_cpu(access.control);
        if (control == 0)
            return 0;
        if (control & FW_CFG_DMA_CTL_ERROR)
            return -EIO;
    }

    return -ETIMEDOUT;
}

int qemu_ramfb_configure(struct fb_info *info) {
    struct ramfb_config cfg;

    cfg.addr = cpu_to_be64(info->screen_phys);
    cfg.fourcc = cpu_to_be32(DRM_FORMAT_XRGB8888);
    cfg.flags = cpu_to_be32(0);
    cfg.width = cpu_to_be32(info->width);
    cfg.height = cpu_to_be32(info->height);
    cfg.stride = cpu_to_be32(info->pitch);

    return qemu_fw_cfg_write_file("etc/ramfb", &cfg, sizeof(cfg));
}

static int qemu_fw_cfg_probe(struct platform_device *pdev) {
    virt_addr_t base;
    char sig[4];

    base = platform_ioremap_resource(pdev, 0);
    if (!base)
        return -ENOMEM;

    fw_cfg.base = base;
    fw_cfg_select(FW_CFG_SIGNATURE);
    fw_cfg_read(sig, sizeof(sig));
    if (memcmp(sig, "QEMU", sizeof(sig)) != 0) {
        printk("qemu_fw_cfg: bad signature %c%c%c%c\n", sig[0], sig[1], sig[2], sig[3]);
        iounmap(base, FW_CFG_MMIO_SIZE);
        fw_cfg.base = 0;
        return -ENODEV;
    }

    printk("qemu_fw_cfg: probed\n");
    return 0;
}

static int qemu_fw_cfg_remove(struct platform_device *pdev) {
    (void)pdev;

    if (fw_cfg.base) {
        iounmap(fw_cfg.base, FW_CFG_MMIO_SIZE);
        fw_cfg.base = 0;
    }

    return 0;
}

static const struct of_device_id qemu_fw_cfg_of_match[] = {
    {.compatible = "qemu,fw-cfg-mmio"},
    {/* sentinel */},
};

static struct platform_driver qemu_fw_cfg_driver = {
    .name = "qemu-fw-cfg",
    .probe = qemu_fw_cfg_probe,
    .remove = qemu_fw_cfg_remove,
    .driver =
        {
            .of_match_table = qemu_fw_cfg_of_match,
        },
};

static int qemu_fw_cfg_driver_init(void) {
    return platform_driver_register(&qemu_fw_cfg_driver);
}

static void qemu_fw_cfg_driver_exit(void) {
    platform_driver_unregister(&qemu_fw_cfg_driver);
}

subsys_initcall(qemu_fw_cfg_driver_init);
module_exit(qemu_fw_cfg_driver_exit, ".exitcall");
