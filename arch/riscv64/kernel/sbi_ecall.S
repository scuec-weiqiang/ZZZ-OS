.section .text
.globl sbi_ecall
.type sbi_ecall, @function
sbi_ecall:
    /*
     * C ABI on entry:
     *   a0=ext, a1=fid, a2=arg0, a3=arg1, a4=arg2, a5=arg3, a6=arg4, a7=arg5
     * SBI ABI:
     *   a7=ext, a6=fid, a0-a5=args
     * Return:
     *   a0=error, a1=value
     */
    mv t0, a0
    mv t1, a1
    mv a0, a2
    mv a1, a3
    mv a2, a4
    mv a3, a5
    mv a4, a6
    mv a5, a7
    mv a6, t1
    mv a7, t0
    ecall
    ret
.size sbi_ecall, . - sbi_ecall
