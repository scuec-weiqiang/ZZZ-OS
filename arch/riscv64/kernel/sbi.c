#include <asm/sbi.h>

long sbi_probe_extension(long extid)
{
    struct sbiret ret;

    ret = sbi_ecall(SBI_EXT_BASE, SBI_EXT_BASE_PROBE_EXT,
                    (unsigned long)extid, 0, 0, 0, 0, 0);
    if (ret.error != SBI_SUCCESS) {
        return ret.error;
    }

    return ret.value;
}

long sbi_hart_start(unsigned long hartid, unsigned long start_addr,
                    unsigned long opaque)
{
    struct sbiret ret;

    ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_START,
                    hartid, start_addr, opaque, 0, 0, 0);

    return ret.error;
}

long sbi_hart_stop(void)
{
    struct sbiret ret;

    ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STOP, 0, 0, 0, 0, 0, 0);

    return ret.error;
}

long sbi_hart_get_status(unsigned long hartid)
{
    struct sbiret ret;

    ret = sbi_ecall(SBI_EXT_HSM, SBI_EXT_HSM_HART_STATUS,
                    hartid, 0, 0, 0, 0, 0);
    if (ret.error != SBI_SUCCESS) {
        return ret.error;
    }

    return ret.value;
}

long sbi_ipi_send(unsigned long hart_mask, unsigned long hart_mask_base) {
    struct sbiret ret;

    ret = sbi_ecall(SBI_EXT_IPI, SBI_EXT_IPI_SEND,
                    hart_mask, hart_mask_base, 0, 0, 0, 0);

    return ret.error;
}
