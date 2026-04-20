#ifndef __OS_RTC_H
#define __OS_RTC_H
#include <os/types.h>

typedef int (*rtc_read_cb_t)(uint64_t *sec);
extern int rtc_register(rtc_read_cb_t rtc_cb);
extern int rtc_read_time(uint64_t *sec);
extern int rtc_set_time(uint64_t sec);

#endif