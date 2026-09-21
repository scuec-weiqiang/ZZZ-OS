#ifndef __OS_VSPRINTF_H__
#define __OS_VSPRINTF_H__

#include <os/types.h>
#include <stdarg.h>

/*
 * snprintf()/vsnprintf() return the length that would have been produced.
 * scnprintf()/vscnprintf() return the length actually stored, excluding NUL.
 */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
int vscnprintf(char *buf, size_t size, const char *fmt, va_list args);
int snprintf(char *buf, size_t size, const char *fmt, ...);
int scnprintf(char *buf, size_t size, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);

#endif
