/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_TERMIOS_H
#define _UAPI_LINUX_TERMIOS_H

#include <uapi/asm-generic/termios.h>

#define INIT_C_CC {		\
	[VINTR] = 'C'-0x40,	\
	[VQUIT] = '\\'-0x40,	\
	[VERASE] = '\177',	\
	[VKILL] = 'U'-0x40,	\
	[VEOF] = 'D'-0x40,	\
	[VSTART] = 'Q'-0x40,	\
	[VSTOP] = 'S'-0x40,	\
	[VSUSP] = 'Z'-0x40,	\
	[VREPRINT] = 'R'-0x40,	\
	[VDISCARD] = 'O'-0x40,	\
	[VWERASE] = 'W'-0x40,	\
	[VLNEXT] = 'V'-0x40,	\
	INIT_C_CC_VDSUSP_EXTRA	\
	[VMIN] = 1 }
    
#endif /* _UAPI_LINUX_TERMIOS_H */
