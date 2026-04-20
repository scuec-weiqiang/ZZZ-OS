.text
.align 4

.global print_char
print_char:
    @ 保存所有临时寄存器（调用后完全恢复，完美安全）
    push {r0-r3, ip, lr}

    ldr r1, =0x02020000       @ 串口基地址

1: ldr r2, [r1, #0xB4]        @ 读 UTS 状态寄存器
   tst r2, #(1<<4)            @ 检查发送忙位
   bne 1b                    @ 忙则等待

    strb r0, [r1, #0x40]      @ 发送字符到 UTXD

    @ 完全恢复寄存器
    pop {r0-r3, ip, lr}
    mov pc, lr                @ 返回

.global __switch_to
__switch_to:
    stmia r1, {r4-r11, sp, lr}      // 保存旧进程
    
    ldmia r2, {r4-r11, sp, pc}      // 恢复新进程

    @ mov pc, lr                       // 跳转到新进程的 lr
    @ add r3, r1, #4
    @ stmia r3, {r4-r11, sp, lr}

    @ add r3, r2, #4 
    @ ldmia r3, {r4-r11, sp, pc}
