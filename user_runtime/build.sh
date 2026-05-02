arm-none-eabi-as -mcpu=arm7tdmi  arm/crt0.S -o crt0.o
arm-none-eabi-as -mcpu=arm7tdmi  arm/syscall.S -o syscall.o
arm-none-eabi-gcc -mcpu=arm7tdmi -mfloat-abi=soft -marm -c syscalls.c -o syscalls.o