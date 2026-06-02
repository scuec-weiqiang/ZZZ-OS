#ifndef ZZZ_OS_UAPI_FCNTL_DEFS_H
#define ZZZ_OS_UAPI_FCNTL_DEFS_H

/*
 * Shared open/fcntl flag values for both kernel and userspace.
 * Keep these in sync with the syscall ABI exposed by this OS.
 */

#undef O_ACCMODE
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_CREAT
#undef O_EXCL
#undef O_NOCTTY
#undef O_TRUNC
#undef O_APPEND
#undef O_NONBLOCK
#undef O_SYNC
#undef O_DIRECTORY
#undef O_NOFOLLOW
#undef O_CLOEXEC

#define O_ACCMODE   00000003
#define O_RDONLY    00000000
#define O_WRONLY    00000001
#define O_RDWR      00000002
#define O_CREAT     00000100
#define O_EXCL      00000200
#define O_NOCTTY    00000400
#define O_TRUNC     00001000
#define O_APPEND    00002000
#define O_NONBLOCK  00004000
#define O_SYNC      00010000
#define O_DIRECTORY 00200000
#define O_NOFOLLOW  00400000
#define O_CLOEXEC   02000000

#endif
