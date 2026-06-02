ARCH ?= arm

ifeq ($(ARCH),riscv64)
CROSS_COMPILE ?= riscv64-unknown-elf-
CPUFLAGS = -march=rv64gc -mabi=lp64d -mcmodel=medany -fno-omit-frame-pointer
ASFLAGS = $(CPUFLAGS)
else
CROSS_COMPILE ?= arm-none-eabi-
CPUFLAGS = -mcpu=arm7tdmi -mfloat-abi=soft -marm
ASFLAGS = -mcpu=arm7tdmi
endif

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)gcc

CFLAGS = $(CPUFLAGS) -Wall -g -I../include

BUILD_DIR = build/$(ARCH)
CRT0_SRC = $(ARCH)/crt0.S
SYSCALL_SRC = $(ARCH)/syscall.S
OBJS = $(BUILD_DIR)/crt0.o $(BUILD_DIR)/syscall.o $(BUILD_DIR)/syscalls.o

.PHONY: all clean

all: $(OBJS)

$(BUILD_DIR)/crt0.o: $(CRT0_SRC)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/syscall.o: $(SYSCALL_SRC)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/syscalls.o: syscalls.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
