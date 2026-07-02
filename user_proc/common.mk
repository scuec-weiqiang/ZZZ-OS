ARCH ?= riscv64

COMMON_MK_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
ARCH_TOOLCHAIN_MK := $(COMMON_MK_DIR).toolchain/$(ARCH).mk

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
RUNTIME ?= ../../user_runtime
BUILD_DIR ?= build/$(ARCH)
# INSTALL_DIR ?= ../../../linux/tftpboot/mnt/bin
INSTALL_DIR ?= ../../mount/bin
CPPFLAGS += -I../../include
CFLAGS += $(USER_CPUFLAGS) -Wall -g -MMD -MP

ifndef TARGET
$(error TARGET must be set before including ../common.mk)
endif

SRCS ?= $(TARGET).c
OBJS ?= $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))
RUNTIME_OBJS := $(RUNTIME)/build/$(ARCH)/crt0.o \
	$(RUNTIME)/build/$(ARCH)/syscall.o \
	$(RUNTIME)/build/$(ARCH)/syscalls.o
RUNTIME_LD := $(RUNTIME)/$(ARCH)/user.ld

.PHONY: all install clean

ifeq ($(AUTO_INSTALL),0)
all: $(TARGET)
else
all: install
endif

install: $(TARGET)
	@test -d $(INSTALL_DIR) || \
		( echo "install target '$(INSTALL_DIR)' is not a directory; mount the image first" && exit 1 )
	sudo cp $(TARGET) $(INSTALL_DIR)/

$(RUNTIME_OBJS) &: $(RUNTIME)/$(ARCH)/crt0.S $(RUNTIME)/$(ARCH)/syscall.S \
	$(RUNTIME)/syscalls.c $(RUNTIME)/Makefile $(RUNTIME_LD)
	$(MAKE) -C $(RUNTIME) all

$(TARGET): $(OBJS) $(RUNTIME_OBJS) $(RUNTIME_LD)
	$(CC) $(CFLAGS) \
		-nostartfiles \
		-T $(RUNTIME_LD) \
		$(RUNTIME_OBJS) $(OBJS) -lc $(LDLIBS) -lgcc -o $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(OBJS:.o=.d)
