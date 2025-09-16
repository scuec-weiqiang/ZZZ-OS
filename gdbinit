set disassemble-next-line on
layout split
b _start
b init.c:29
add-symbol-file ./user/out/user.elf 0x00010000
target remote :1234
c