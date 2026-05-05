.text
.align 4

.global __aeabi_uidiv
.type __aeabi_uidiv, %function
.global __aeabi_uidivmod
.type __aeabi_uidivmod, %function

__aeabi_uidiv:
__aeabi_uidivmod:
    cmp r1, #0
    beq .Ldivzero

    mov r2, #0
    mov r3, #0
    mov ip, #32

.Ldiv_loop:
    mov r2, r2, lsl #1
    mov r3, r3, lsl #1
    orr r3, r3, r0, lsr #31
    mov r0, r0, lsl #1
    cmp r3, r1
    subcs r3, r3, r1
    orrcs r2, r2, #1
    subs ip, ip, #1
    bne .Ldiv_loop

    mov r0, r2
    mov r1, r3
    bx lr

.Ldivzero:
    mov r0, #0
    mov r1, #0
    bx lr

.size __aeabi_uidiv, . - __aeabi_uidiv
.size __aeabi_uidivmod, . - __aeabi_uidivmod


.text
.align 4

.global __aeabi_ldivmod
.type __aeabi_ldivmod, %function

/*
 * __aeabi_ldivmod(long long n, long long d)
 *
 * input:
 *   r0:r1 = numerator
 *   r2:r3 = denominator
 *
 * output:
 *   r0:r1 = quotient
 *   r2:r3 = remainder
 *
 * little endian ARM EABI:
 *   low word first
 */

__aeabi_ldivmod:
    push {r4-r11, lr}

    /* save denominator */
    mov r4, r2          /* den_lo */
    mov r5, r3          /* den_hi */

    /* sign flags */
    mov r10, #0         /* quotient negative flag */
    mov r11, #0         /* remainder negative flag */

    /* if numerator < 0, negate it */
    cmp r1, #0
    bge 1f

    mvn r0, r0
    mvn r1, r1
    adds r0, r0, #1
    adc r1, r1, #0

    eor r10, r10, #1
    eor r11, r11, #1

1:
    /* if denominator < 0, negate it */
    cmp r5, #0
    bge 2f

    mvn r4, r4
    mvn r5, r5
    adds r4, r4, #1
    adc r5, r5, #0

    eor r10, r10, #1

2:
    /* divide unsigned: numerator r0:r1, denominator r4:r5 */

    /* quotient q = r6:r7 */
    mov r6, #0
    mov r7, #0

    /* remainder rem = r8:r9 */
    mov r8, #0
    mov r9, #0

    mov ip, #64

.Lloop:
    /*
     * shift numerator left, top bit goes into carry
     */
    movs r0, r0, lsl #1
    adc  r1, r1, r1

    /*
     * rem = (rem << 1) | carry
     */
    adcs r8, r8, r8
    adc  r9, r9, r9

    /*
     * q <<= 1
     */
    movs r6, r6, lsl #1
    adc  r7, r7, r7

    /*
     * if rem >= denominator:
     *     rem -= denominator
     *     q |= 1
     */
    cmp r9, r5
    cmpeq r8, r4
    blo .Lskip_sub

    subs r8, r8, r4
    sbc  r9, r9, r5
    orr  r6, r6, #1

.Lskip_sub:
    subs ip, ip, #1
    bne .Lloop

    /*
     * apply quotient sign
     */
    cmp r10, #0
    beq 3f

    mvn r6, r6
    mvn r7, r7
    adds r6, r6, #1
    adc r7, r7, #0

3:
    /*
     * apply remainder sign
     */
    cmp r11, #0
    beq 4f

    mvn r8, r8
    mvn r9, r9
    adds r8, r8, #1
    adc r9, r9, #0

4:
    mov r0, r6
    mov r1, r7
    mov r2, r8
    mov r3, r9

    pop {r4-r11, pc}

.size __aeabi_ldivmod, . - __aeabi_ldivmod


.global __aeabi_uldivmod
.type __aeabi_uldivmod, %function

__aeabi_uldivmod:
    push {r4-r11, lr}

    mov r4, r2
    mov r5, r3

    mov r6, #0
    mov r7, #0
    mov r8, #0
    mov r9, #0

    mov ip, #64

.Luloop:
    movs r0, r0, lsl #1
    adc  r1, r1, r1

    adcs r8, r8, r8
    adc  r9, r9, r9

    movs r6, r6, lsl #1
    adc  r7, r7, r7

    cmp r9, r5
    cmpeq r8, r4
    blo .Luskip_sub

    subs r8, r8, r4
    sbc  r9, r9, r5
    orr  r6, r6, #1

.Luskip_sub:
    subs ip, ip, #1
    bne .Luloop

    mov r0, r6
    mov r1, r7
    mov r2, r8
    mov r3, r9

    pop {r4-r11, pc}

.size __aeabi_uldivmod, . - __aeabi_uldivmod
