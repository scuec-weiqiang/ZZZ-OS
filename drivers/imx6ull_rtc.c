/**
 * @FilePath: /ZZZ-OS/drivers/imx6ull_rtc.c
 * @Description: i.MX6ULL internal SNVS RTC driver (minimal)
 */
#include <os/types.h>
#include <os/of.h>
#include <os/printk.h>
#include <os/mm.h>
#include <os/platform_device.h>
#include <os/timekeeping.h>

#define SNVS_LPCR      0x38
#define SNVS_LPSRTCMR  0x50
#define SNVS_LPSRTCLR  0x54

#define SNVS_LPCR_SRTC_ENV (1U << 0)

static virt_addr_t snvs_base;
static int snvs_ready;

static inline u32 snvs_readl(u32 off) {
    volatile u32 *addr = (volatile u32 *)(snvs_base + off);
    return *addr;
}

static inline void snvs_writel(u32 off, u32 val) {
    volatile u32 *addr = (volatile u32 *)(snvs_base + off);
    *addr = val;
}

/*
 * SNVS SRTC counter is a 47-bit counter running at 32.768KHz.
 * Convert to seconds by right-shifting 15 bits.
 */
static u32 imx6ull_rtc_read_seconds(void) {
    u32 msb1, msb2, lsb;
    u64 ticks;

    do {
        msb1 = snvs_readl(SNVS_LPSRTCMR);
        lsb = snvs_readl(SNVS_LPSRTCLR);
        msb2 = snvs_readl(SNVS_LPSRTCMR);
    } while (msb1 != msb2);

    ticks = (((u64)msb1) << 32) | (u64)lsb;
    return (u32)(ticks >> 15);
}

static int imx6ull_rtc_read(u64 *sec) {
    if (!snvs_ready || sec == NULL) {
        return -1;
    }

    *sec = (u64)imx6ull_rtc_read_seconds();
    return 0;
}

static void imx6ull_rtc_enable(void) {
    u32 lpcr = snvs_readl(SNVS_LPCR);
    lpcr |= SNVS_LPCR_SRTC_ENV;
    snvs_writel(SNVS_LPCR, lpcr);
}

static int imx6ull_rtc_probe(struct platform_device *pdev) {
    u32 now;

    snvs_base = platform_ioremap_resource(pdev, 0);
    printk("imx6ull_rtc: snvs_base=%xu\n", snvs_base);
    if (!snvs_base) {
        return -1;
    }

    imx6ull_rtc_enable();
    snvs_ready = 1;
    // if (rtc_register(imx6ull_rtc_read) < 0) {
    //     printk("imx6ull_rtc: register rtc callback failed\n");
    // }
    now = imx6ull_rtc_read_seconds();
    printk("imx6ull_rtc: unix=%du\n", now);
    return 0;
}

static int imx6ull_rtc_remove(struct platform_device *pdev) {
    (void)pdev;
    if (snvs_base) {
        iounmap(snvs_base, 0x4000);
    }
    snvs_ready = 0;
    snvs_base = 0;
    return 0;
}

static struct of_device_id imx6ull_rtc_of_match[] = {
    { .compatible = "fsl,imx6ull-snvs-rtc" },
    { /* sentinel */ }
};

static struct platform_driver imx6ull_rtc_driver = {
    .name = "imx6ull_rtc",
    .probe = imx6ull_rtc_probe,
    .remove = imx6ull_rtc_remove,
    .driver = {
        .of_match_table = imx6ull_rtc_of_match,
    }
};

module_platform_driver(imx6ull_rtc_driver);
