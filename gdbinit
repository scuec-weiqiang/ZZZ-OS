set disassemble-next-line on
layout split
b _start
b boot.S:56
b init.c:46
target remote :1234
c