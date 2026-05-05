.text
.align 2

.global __switch_to
__switch_to:
    sd ra, 0(a1)
    sd sp, 8(a1)
    sd s0, 16(a1)
    sd s1, 24(a1)
    sd s2, 32(a1)
    sd s3, 40(a1)
    sd s4, 48(a1)
    sd s5, 56(a1)
    sd s6, 64(a1)
    sd s7, 72(a1)
    sd s8, 80(a1)
    sd s9, 88(a1)
    sd s10, 96(a1)
    sd s11, 104(a1)

    ld ra, 0(a2)
    ld sp, 8(a2)
    ld s0, 16(a2)
    ld s1, 24(a2)
    ld s2, 32(a2)
    ld s3, 40(a2)
    ld s4, 48(a2)
    ld s5, 56(a2)
    ld s6, 64(a2)
    ld s7, 72(a2)
    ld s8, 80(a2)
    ld s9, 88(a2)
    ld s10, 96(a2)
    ld s11, 104(a2)
    ret
