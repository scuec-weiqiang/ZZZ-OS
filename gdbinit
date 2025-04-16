set disassemble-next-line on
layout split
b _start
b boot.s:31
target remote :1234
c