#ifndef __ASM_RISCV64_SBI_H
#define __ASM_RISCV64_SBI_H

#include <os/types.h>

struct sbiret {
    long error;
    long value;
};

#define SBI_SUCCESS                 0
#define SBI_ERR_FAILED             -1
#define SBI_ERR_NOT_SUPPORTED      -2
#define SBI_ERR_INVALID_PARAM      -3
#define SBI_ERR_DENIED             -4
#define SBI_ERR_INVALID_ADDRESS    -5
#define SBI_ERR_ALREADY_AVAILABLE  -6
#define SBI_ERR_ALREADY_STARTED    -7
#define SBI_ERR_ALREADY_STOPPED    -8

#define SBI_EXT_BASE               0x10
#define SBI_EXT_HSM                0x48534D

#define SBI_EXT_BASE_PROBE_EXT     3

#define SBI_EXT_HSM_HART_START     0
#define SBI_EXT_HSM_HART_STOP      1
#define SBI_EXT_HSM_HART_STATUS    2
#define SBI_EXT_HSM_HART_SUSPEND   3

#define SBI_EXT_IPI        0x735049
#define SBI_EXT_IPI_SEND   0

struct sbiret sbi_ecall(unsigned long ext, unsigned long fid,
                        unsigned long arg0, unsigned long arg1,
                        unsigned long arg2, unsigned long arg3,
                        unsigned long arg4, unsigned long arg5);

long sbi_probe_extension(long extid);
long sbi_hart_start(unsigned long hartid, unsigned long start_addr,
                    unsigned long opaque);
long sbi_hart_stop(void);
long sbi_hart_get_status(unsigned long hartid);
long sbi_ipi_send(unsigned long hart_mask, unsigned long hart_mask_base);

#endif
