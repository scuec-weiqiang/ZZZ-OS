.section .text
.global _start
_start:
  li a7, 93   # syscall_exit
  li a0, 0
  ecall