CC = arm-none-eabi-gcc
AS = arm-none-eabi-as

CPUFLAGS = -mcpu=arm7tdmi -mfloat-abi=soft -marm
CFLAGS = $(CPUFLAGS) -Wall -g
ASFLAGS = -mcpu=arm7tdmi

OBJS = crt0.o syscall.o syscalls.o

.PHONY: all clean

all: $(OBJS)

crt0.o: arm/crt0.S
	$(AS) $(ASFLAGS) $< -o $@

syscall.o: arm/syscall.S
	$(AS) $(ASFLAGS) $< -o $@

syscalls.o: syscalls.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)
