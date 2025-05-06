set disassemble-next-line on
layout split
b _start
b init.c:29
target remote :1234
c