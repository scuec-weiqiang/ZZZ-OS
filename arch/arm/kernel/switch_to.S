.text
.align 4

.global __switch_to
__switch_to:
    stmia r1, {r4-r11, sp, lr}      // 保存旧进程
    cpsie i
    ldmia r2, {r4-r11, sp, pc}      // 恢复新进程
