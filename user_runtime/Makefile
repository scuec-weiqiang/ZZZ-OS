ARCH ?= arm

ARCH_TOOLCHAIN_MK := ../user_proc/.toolchain/$(ARCH).mk

ifneq ($(wildcard $(ARCH_TOOLCHAIN_MK)),)
include $(ARCH_TOOLCHAIN_MK)
endif

ifndef CROSS_COMPILE
$(error Missing CROSS_COMPILE; set it in the parent Makefile or via $(ARCH_TOOLCHAIN_MK))
endif

ifndef USER_CPUFLAGS
$(error Missing USER_CPUFLAGS; set it in the parent Makefile or via $(ARCH_TOOLCHAIN_MK))
endif

USER_ASFLAGS ?= $(USER_CPUFLAGS)

CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)gcc

CFLAGS := $(USER_CPUFLAGS) -Wall -g -I../include
ASFLAGS := $(USER_ASFLAGS)

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
