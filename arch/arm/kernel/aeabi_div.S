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
