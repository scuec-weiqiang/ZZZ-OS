/**
 * @FilePath: /ZZZ-OS/kernel/timekeeping.c
 * @Description: minimal timekeeping based on arch timer + optional RTC boot time
 */
#include <os/printk.h>
#include <os/time.h>
#include <os/types.h>
#include <os/utils.h>

#define NSEC_PER_SEC 1000000000ULL

static uint8_t tk_inited;
static uint64_t boot_mono_ns;
static uint64_t boot_real_sec;
static clocksource_counter_cb_t cs_counter_cb;
static clocksource_frequency_cb_t cs_frequency_cb;
static rtc_boot_seconds_cb_t rtc_boot_cb;

int timekeeping_register_clocksource(clocksource_counter_cb_t counter_cb,
                                     clocksource_frequency_cb_t frequency_cb) {
    if (counter_cb == NULL || frequency_cb == NULL) {
        return -1;
    }

    cs_counter_cb = counter_cb;
    cs_frequency_cb = frequency_cb;
    return 0;
}

int timekeeping_register_rtc(rtc_boot_seconds_cb_t rtc_cb) {
    if (rtc_cb == NULL) {
        return -1;
    }

    rtc_boot_cb = rtc_cb;
    return 0;
}

uint64_t monotonic_ns(void) {
    uint64_t cnt;
    uint32_t freq;
    uint64_t sec;
    uint64_t rem_ns;
    uint32_t rem;

    if (cs_counter_cb == NULL || cs_frequency_cb == NULL) {
        return 0;
    }

    cnt = cs_counter_cb();
    freq = cs_frequency_cb();
    if (freq == 0) {
        return 0;
    }

    sec = divmod_u64(cnt, freq, &rem);
    rem_ns = div_u64((uint64_t)rem * NSEC_PER_SEC, freq);
    return sec * NSEC_PER_SEC + rem_ns;
}

void timekeeping_init(void) {
    uint64_t sec = 0;

    boot_mono_ns = monotonic_ns();
    if (rtc_boot_cb && rtc_boot_cb(&sec) == 0) {
        boot_real_sec = sec;
        printk("timekeeping: rtc boot seconds=%du\n", (uint32_t)sec);
    } else {
        boot_real_sec = 0;
        printk("timekeeping: rtc unavailable, realtime starts from 0\n");
    }
    tk_inited = 1;
}

uint64_t realtime_sec(void) {
    uint64_t now_ns;
    uint64_t delta_ns;

    if (!tk_inited) {
        timekeeping_init();
    }

    now_ns = monotonic_ns();
    delta_ns = now_ns - boot_mono_ns;
    return boot_real_sec + div_u64(delta_ns, (uint32_t)NSEC_PER_SEC);
}

uint64_t realtime_ns(void) {
    uint64_t now_ns;
    uint64_t delta_ns;

    if (!tk_inited) {
        timekeeping_init();
    }

    now_ns = monotonic_ns();
    delta_ns = now_ns - boot_mono_ns;
    return boot_real_sec * NSEC_PER_SEC + delta_ns;
}
