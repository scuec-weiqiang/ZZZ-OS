ARCH ?= arm
RUNTIME = ../../user_runtime

ifeq ($(ARCH),riscv64)
CROSS_COMPILE ?= riscv64-unknown-elf-
CPUFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany -fno-omit-frame-pointer
else
CROSS_COMPILE ?= arm-none-eabi-
CPUFLAGS = -mcpu=arm7tdmi -mfloat-abi=soft -marm
endif

CC = $(CROSS_COMPILE)gcc
CFLAGS = $(CPUFLAGS) -Wall -g -I../../include
RUNTIME_OBJS = $(RUNTIME)/build/$(ARCH)/crt0.o \
	$(RUNTIME)/build/$(ARCH)/syscall.o \
	$(RUNTIME)/build/$(ARCH)/syscalls.o
RUNTIME_LD = $(RUNTIME)/$(ARCH)/user.ld


TARGET = hello
OBJS = $(TARGET).o

.PHONY: all clean

all: $(TARGET)
	sudo cp $(TARGET) ../../../linux/tftpboot/mnt/bin

$(RUNTIME_OBJS) &: $(RUNTIME)/$(ARCH)/crt0.S $(RUNTIME)/$(ARCH)/syscall.S \
	$(RUNTIME)/syscalls.c $(RUNTIME)/Makefile
	$(MAKE) -C $(RUNTIME) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) all

$(TARGET): $(RUNTIME_OBJS) $(RUNTIME_LD) $(OBJS)
	$(CC) $(CFLAGS) \
		-nostartfiles \
		-T $(RUNTIME_LD) \
		$(RUNTIME_OBJS) \
		$(OBJS) -lc -lgcc -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
